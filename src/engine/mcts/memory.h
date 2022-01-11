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


#ifndef FIREFLY_MEMORY_H
#define FIREFLY_MEMORY_H

#include <cstdint>
#include <vector>
#include <mutex>

namespace mcts
{
    struct node;
};

/*
 * This memory manager works by allocating large chunks of memory (1MB by default)
 * and dynamically allocating every node and its edges in contiguous memory.
 *
 * Freeing is done by copying all important nodes to the beginning of all memory,
 * overwriting any dead nodes and making all memory immediately following
 * blocks[0] + sizeof(copied_nodes) available for new allocations.
 */
struct memory {


    /*
     * Initializes the memory manager with a block size and allocates an initial block.
     */
    memory(size_t max_nn_batch_size=2048, size_t block_size = 8388608 /* 8 MiB */);
    ~memory();

    /*
     * The new root is copied to blocks[0] + 0 and all of its children are directly after it,
     * their children after that, etc.
     * This just leaves the parents of the new root, which are still needed, so copy them over to erased_parents
     *
     * NOTE: Any tree traversal while this function is working is undefined,
     * and probably going to lead to a segmentation fault.
     *
     */
    mcts::node* free_unused(mcts::node *new_root, std::vector<std::unique_ptr<mcts::node>> &erased_parents);

    /*
     * Resets all indexes.
     * The next allocation will return blocks[0] + 0
     * Does not actually free any memory to the system.
     */
    void clear();

    /*
     *  mallocs a new block of memory
     *
     *  Returns true if a block was successfully allocated, false otherwise.
     */
    bool sys_malloc_new_block();

    /*
     * Releases all allocated memory back to the system
     */
    void sys_free_memory();


    /*
     * Allocates the memory for one node and all of its edges.
     */
    mcts::node* allocate_fused_node(size_t edge_count);



    inline const mcts::node* get_root()
    {
        return (mcts::node*)blocks[0];
    }

    /*
     * Returns a transposition if one is stored and evaluated and the repetitions are the same
     * The node it returns is locked, when the necessary data is retrieved, it should be unlocked
     */
    mcts::node* transposition_check(mcts::node* node);


    // Returns a batch to be used for NN input planes
    float* get_batch_memory();
    void release_batch_memory(float* memory);



private:

    std::vector<float*> batch_intermediate_memory;
    std::mutex batch_intermediate_memory_lock;

    const int get_parent_block_index(const mcts::node* node);
    mcts::node* allocate_fused_node_lockless(size_t edge_count);

    std::mutex transposition_table_lock;
    std::unordered_map<uint64_t, mcts::node*> transposition_table;

    size_t current_block, index_in_block, block_size, max_nn_batch_size;
    std::vector<std::byte*> blocks;
    std::mutex memory_lock;
};

#endif //FIREFLY_MEMORY_H
