/*
    Firefly Chess Engine
    Copyright (C) 2022  Ognyan Mirev

    This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "memory.h"

#include <chrono>
#include "node.h"

using namespace std;

memory::memory(size_t max_nn_batch_size, size_t block_size) : block_size(block_size),
index_in_block(0), current_block(0), max_nn_batch_size(max_nn_batch_size)
{
    blocks.reserve(256);
    sys_malloc_new_block();

    transposition_table.reserve(1e+8);
}

memory::~memory() {
    sys_free_memory();
}

mcts::node* memory::allocate_fused_node_lockless(size_t edge_count)
{
    size_t required_memory = sizeof(mcts::node) + sizeof(mcts::edge) * edge_count;

    if ((index_in_block + required_memory) >= block_size)
    {
        current_block++;
        index_in_block = 0;
    }

    auto memory = blocks[current_block] + index_in_block;
    index_in_block += required_memory;

    return (mcts::node*)memory;
}


mcts::node* memory::allocate_fused_node(size_t edge_count)
{
    size_t required_memory = sizeof(mcts::node) + sizeof(mcts::edge) * edge_count;

    std::lock_guard lock(memory_lock);

    if ((index_in_block + required_memory) >= block_size)
    {
        current_block++;
        index_in_block = 0;

        // TODO: The engine doesn't necessarily need to crash if it runs out of memory.
        // One solution would be to halt all tree traversal and do a free operation.
        // Another solution would be to stop searching and just return the best move found so far.
        if (current_block >= blocks.size()) {
            if (!sys_malloc_new_block())
            {
                throw std::logic_error("[memory] malloc failed.");
            }
        }
    }

    auto memory = blocks[current_block] + index_in_block;
    index_in_block += required_memory;


    return (mcts::node*)memory;
}



mcts::node* memory::free_unused(mcts::node* new_root, std::vector<std::unique_ptr<mcts::node>>& erased_parents)
{
    auto time_start = chrono::high_resolution_clock::now();

    /*
     *  Backup the history of the position in an external memory structure
     *  This is necessary for threefold repetition detection and EncodePositionForNN history planes.
     *  Edges are not preserved.
     */

    static vector<mcts::node*> history;


    auto node = new_root->parent;

    while (node)
    {
        history.push_back(node);

        if (node == get_root()) break;

        node = node->parent;

    }

    while(!history.empty())
    {
        node = history.back();
        history.pop_back();
        erased_parents.push_back(make_unique<mcts::node>(*node));

        erased_parents.back()->edge_count = 0;
        erased_parents.back()->viable_edges = 0;

        if (!history.empty())
            history.back()->parent = erased_parents.back().get();
        else
            new_root->parent = erased_parents.back().get();
    }




    //region Shift all still viable nodes to blocks[0] + 0

    size_t previous_bytes = current_block * block_size + index_in_block;
    // Reset indices
    clear();


    auto new_root_memory = allocate_fused_node_lockless(new_root->edge_count);
    // Copy root to start
    memcpy(new_root_memory, new_root, sizeof(mcts::node) + sizeof(mcts::edge) * new_root->edge_count);


    static vector<mcts::node*> nodes_to_fix;


    // Parse all nodes that need to be moved

    size_t last_layer_size = 0;
    // Fix references to root in children
    for (auto& i : *new_root_memory) {
        if (i.get_node()) {
            i.get_node()->parent = new_root_memory;
            nodes_to_fix.push_back(i.node_);
            last_layer_size++;
        }
    }

    if (!last_layer_size)
        return new_root_memory;


    static vector<vector<mcts::node*>> partitioned_nodes_to_fix;
    partitioned_nodes_to_fix.resize(blocks.size());

    for (auto& i : nodes_to_fix)
        partitioned_nodes_to_fix[get_parent_block_index(i)].push_back(i);

    // Parse all nodes in subtree, layer by layer
    while (last_layer_size)
    {
        auto start = nodes_to_fix.size() - last_layer_size;
        auto end = nodes_to_fix.size();
        last_layer_size = 0;

        for (; start < end; start++)
        {
            for (auto& i : *nodes_to_fix[start])
            {
                node = i.get_node();
                if (node) {
                    nodes_to_fix.push_back(node);
                    partitioned_nodes_to_fix[get_parent_block_index(node)].push_back(node);
                    last_layer_size++;
                }
            }
        }
    }
    cout << "info Copying " << nodes_to_fix.size() << " nodes." << endl;


    //transposition_table.clear();

    /*
     * Since the tree is expected to grow unpredictably, it's necessary to sort the addresses in ascending order.
     * Otherwise, nodes at lower addresses would likely get overwritten by nodes at higher addresses that just
     * happened to be added to the queue first.
     *
     * Or it would work like that, if malloc returned ascending addresses, however since (blocks[2] < blocks[0])
     * may be true, nodes must be sorted by their block indices primarily and by their addresses secondarily.
     */
    for (auto& i : partitioned_nodes_to_fix)
        sort(i.begin(), i.end());

    // TODO: Nodes occupying contiguous memory could be fused and copied in a single memcpy call
    // This might be a performance improvement.

    // Do the actual copying
    for (auto& partition : partitioned_nodes_to_fix) {
        for (int i = 0; i < partition.size(); i++) {
            node = partition[i];
            auto memory = allocate_fused_node_lockless(node->edge_count);

            transposition_table[node->board.hash()] = memory;
            node->copy_to(memory);
        }
        partition.clear();
    }
    nodes_to_fix.clear();

    size_t current_bytes = current_block * block_size + index_in_block;

    cout << "info [memory] Freed " << (previous_bytes - current_bytes) / 1024 << " kb of memory in " <<
         chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - time_start).count()  << "ms." << endl;

    return new_root_memory;
    //endregion
}


void memory::clear() {
    current_block = 0;
    index_in_block = 0;
    transposition_table.clear();
}


bool memory::sys_malloc_new_block()
{
    auto memory = malloc(block_size);
    if (memory == nullptr) return false;
    blocks.push_back((byte*)memory);
    return true;
}

void memory::sys_free_memory() {
    for (auto& i : blocks)
        free(i);
    blocks.clear();

    for (auto& i : batch_intermediate_memory)
        free(i);

    batch_intermediate_memory.clear();
}


const int memory::get_parent_block_index(const mcts::node *node)
{
    byte* n = (byte*)node;
    for (int i = 0; i < blocks.size(); i++)
    {
        if (blocks[i] <= n && (blocks[i] + block_size) > n)
            return i;
    }

    throw std::logic_error("[memory fault] node is not in any block.");
}


mcts::node *memory::transposition_check(mcts::node *node) {

    mcts::node* result;
    {
        std::lock_guard lock(transposition_table_lock);
        result = transposition_table[node->board.hash()];

        //|| result->repetitions != node->repetitions
        if (!result || result->board != node->board) {
            result = node;
            return nullptr;
        }

        //if (!result->evaluated)
        //    return nullptr;
    }
    //result->lock();
    return result;
}


float* memory::get_batch_memory()
{
    std::lock_guard lock(batch_intermediate_memory_lock);

    if (!batch_intermediate_memory.empty())
    {
        auto memory = batch_intermediate_memory.back();
        batch_intermediate_memory.pop_back();
        return memory;
    }

    return (float*)malloc(max_nn_batch_size * 112 * 8 * 8 * sizeof(float));
}

void memory::release_batch_memory(float* memory)
{
    std::lock_guard lock(batch_intermediate_memory_lock);
    batch_intermediate_memory.push_back(memory);
}




