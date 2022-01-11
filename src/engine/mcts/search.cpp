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


#include "search.h"
#include <queue>
#include <cstring>
#include <algorithm>
#include <testing/chess_gui.h>
#include <csignal>
#include <random>
#include <fstream>
#include <utils/utils.h>

using namespace std;


std::atomic<int> solved_nodes = 0;


std::atomic<int> hash_collisions;
std::atomic<int> num_transpositions;
std::atomic<int> batches;
mcts::node* current_root;

mcts::search::search(cxxopts::ParseResult& options) :
thread_count(options["t"].as<int>()), c_puct(options["c"].as<float>()), c_puct_root(options["c_puct_root"].as<float>()),
net_manager(options), dirichlet_epsilon(options["dirichlet_epsilon"].as<float>()), dirichlet_alpha(options["dirichlet_alpha"].as<float>()),
deallocation_factor(options["deallocation_factor"].as<int>()), deallocation_minimum(options["deallocation_minimum"].as<int>()),
memory_(options["max_batch_size"].as<int>())
{
    working = true;
    paused = true;


#ifdef SYNCHRONOUS_INFERENCE
    for (int i = 0; i < thread_count; i++)
        threads.emplace_back(std::jthread(&mcts::search::expand_tree_puct_worker_synchronous, this));
#else
    for (int i = 0; i < thread_count; i++)
        threads.emplace_back(jthread(&mcts::search::expand_tree_puct_worker, this));
#endif
}

mcts::search::~search()
{
    {
        std::lock_guard l(pausing_mutex);
        working = false;
        paused = false;
        paused_cv.notify_all();
    }
    threads.clear();
}

bool mcts::search::initialize(string const& fen)
{
    memory_.clear();

    game_has_ended = false;
    net_manager.memory_ = &memory_;


    shared_batch = get_buffer();

    past_roots.clear();
    past_roots.reserve(2048);

    if (!root_board.from_fen(fen))
        return false;

    chess::movegen_result moves;

    if (root_board.generate_moves(moves) != chess::game_state::playing)
        return false;


    current_root = memory_.allocate_fused_node(moves.moves_count);
    new (current_root) node(root_board, moves, nullptr);

    ::current_root = current_root;


    return true;
}

void mcts::search::free_memory()
{
    if (!current_root->reversible_move &&
        approximate_nodes_to_clear >= deallocation_minimum &&
        approximate_nodes_to_clear > (current_root->visit_count * deallocation_factor)){
        current_root = memory_.free_unused(current_root, past_roots);
        approximate_nodes_to_clear = 0;
    }
}


bool mcts::search::make_move_external(string const& uci_move)
{
    for (auto& i : *current_root)
    {
        if (i.move.to_uci_move() == uci_move)
        {
            advance_root(&i);
            return true;
        }
    }
    return false;
}

void mcts::search::advance_root(mcts::edge * edge_to_new_root)
{
    auto& glog = logging::log("graph");

    if (glog.initialized)
    {
        auto this_id = current_root->board.hash();


        if (!current_root->parent)
        {
            glog << this_id << " [label=\"ROOT NODE\n" <<
                 std::to_string(current_root->Q_) << '\n' <<
                 std::to_string(current_root->visit_count);

            glog << "\"];\n";
        }

        int count = 0;

        bool added_edge_to_new_root = false;

        auto print_child = [&](mcts::edge& i)
        {
            uint64_t child_id;
            if (i.is_expanded()) {

                if (i.is_terminal())
                {
                    auto b = current_root->board;
                    b.make_move(i.move);
                    child_id = b.hash();
                }
                else child_id = i.get_node()->board.hash();

                glog << child_id << " [";
                if (&i == edge_to_new_root) {
                    glog << "color=\"red\" ";
                    added_edge_to_new_root = true;
                }
                glog << "label=\"Move: " <<
                     i.move.to_uci_move() << '\n';

                if (i.is_terminal())
                    glog << "Terminal: True\nValue: " << i.terminal_value;
                else
                {
                    glog << "Terminal: False\nPrior: " << i.P_ << '\n' <<
                         "Value: " << i.get_value() << '\n' <<
                         "Visits: " << i.get_node()->visit_count << '\n' <<
                         "Solved: " << int(i.get_node()->solution) << '\n' <<
                         "Repetitions: " << i.get_node()->repetitions << '\n' <<
                         "Moves left: " << i.get_node()->moves_left << '\n'
#ifdef DEBUG_CHECKS
                        << "Transposition: " << i.get_node()->transposition
#endif
                            ;
                }
                glog << "\"];\n";
            }
            else
            {
                auto b = current_root->board;
                b.make_move(i.move);
                child_id = b.hash();

                glog << child_id << " [label=\"Move: " <<
                     i.move.to_uci_move() << '\n';
                glog << "Prior: " << i.P_ << '\n'
                     << "Unexpanded.";

                glog << "\"];\n";
            }


            glog << this_id << " -> " << child_id << ";\n";
        };
        for (auto& i : *current_root)
        {
            if (count++ == 5)break;
            print_child(i);
        }

        if (!added_edge_to_new_root)
            print_child(*edge_to_new_root);


        glog.ofs.flush();
    }

    moves_played.push_back(edge_to_new_root->move);

    if (edge_to_new_root->is_terminal())
    {
        cout << "info selected terminal node" << endl;
        game_has_ended = true;
        //current_root = (mcts::node*)edge_to_new_root;

        past_roots.push_back(std::make_unique<mcts::node>(*current_root));
        current_root->board.make_move(edge_to_new_root->move);
        current_root->Q_ = edge_to_new_root->terminal_value;
        current_root->parent = past_roots.back().get();
        return;
    }


    if (!edge_to_new_root->is_expanded()) {
        if (!edge_to_new_root->expand(current_root, memory_))
        {
            cout << "info selected unexplored terminal node" << endl;
            game_has_ended = true;
            current_root = (mcts::node*)edge_to_new_root;
            return;
        }
    }


    this->current_root = edge_to_new_root->get_node();

    this->approximate_nodes_to_clear += current_root->parent->visit_count - current_root->visit_count;

    ::current_root = current_root;
    cout << "info Selected: " << edge_to_new_root->move.to_uci_move() << "  value = " << edge_to_new_root->get_value() << endl;
}


mcts::edge* mcts::search::best_move() const
{
    // Sort edges by value in ascending order
    std::sort(current_root->begin(), current_root->end(), [](auto& a, auto& b)
    {
        return a.get_value() > b.get_value();
    });

    auto best_edge = current_root->begin();

    return best_edge;

    // TODO: Make this work.

    /*
     * If the position is winning, the engine should prioritize moves with lower moves_left
     * otherwise it just maneuvers its pieces for no real reason.
     * If the position is losing, prioritizing moves with high moves_left is preferable,
     * as it allows the opponent more opportunities to blunder.
     *
     * That prioritization is basically what this code tries to do.
     */




    if (best_edge->is_terminal()) return best_edge;

    auto best_value = best_edge->get_value();

    if (best_value < 0.9f) return best_edge;

    auto first_value = best_value;

    auto bml = best_edge->get_node() ? best_edge->get_node()->moves_left : 1;

    if (bml == 1) return best_edge;

    for (auto it = current_root->begin() + 1; it != current_root->end(); it++)
    {
        auto value = it->get_value();

        // If the difference is more than .05, abort
        if (abs(first_value - value) > .01f) break;

        auto moves_left = it->get_node() ? it->get_node()->moves_left : 1;

        if ((best_value / bml) < (value / moves_left))
        {
            best_value = value;
            bml = moves_left;
            best_edge = it;
        }
    }

    cout << "info Returning move idx: " << best_edge - current_root->begin() << endl;
    return best_edge;
}


bool mcts::search::prepare_search()
{
    if (current_root->edge_count == 1)
        return false;

    if (current_root->is_solved()) {

        cout << "info Position is solved, value = " << current_root->average_value() << endl;
        return false;
    }
    if (!current_root->evaluated)
    {
#ifdef SYNCHRONOUS_INFERENCE
        net_manager.blocking_inference({current_root});
        current_root->lock_count = 0;
        current_root->locking_tid = -1;
#else
        net_manager.add_to_eval_queue(current_root);
        net_manager.wait_for_node_evaluation(current_root);
#endif
    }



    /*
    for (auto& i : *current_root)
    {
        cout << i.move.to_uci_move() << "  |  " << i.P_ << endl;
    }
     */

    //region Add Dirichlet noise

    if (dirichlet_epsilon != 0) {
        float noise[1024];

        float total = 0;

        std::gamma_distribution<float> gamma_dist(dirichlet_alpha, 1);

        for (int i = 0; i < current_root->edge_count; i++) {
            auto v = gamma_dist(get_rng());
            noise[i] = v;
            total += v;
        }

        if (total > std::numeric_limits<float>::min()) {
            int i = 0;
            for (auto &edge: *current_root) {
                edge.P_ = edge.P_ * (1 - dirichlet_epsilon) + dirichlet_epsilon * noise[i++] / total;
            }
        }
    }


    return true;
}

void mcts::search::expand_tree_puct_worker_synchronous()
{
    static std::atomic<uint8_t> next_tid = 0;

    mcts::node::thread_id = next_tid++;

    if (mcts::node::thread_id == 255)
        mcts::node::thread_id = next_tid++;


    mcts::node* edge_parent;
    mcts::node* next_node;
    mcts::edge* selected_edge;

    std::unique_lock pausing_lock(pausing_mutex);

    pausing_lock.unlock();

    while (working) {

        if (paused) {
            pausing_lock.lock();
            paused_cv.wait(pausing_lock, [this]() {
                return !paused;
            });
            pausing_lock.unlock();
            if (!working) break;
        }

        working_threads++;

        int n_selection_fails = 0;

        while (!paused) {

            edge_parent = current_root;

            edge_parent->lock();
            selected_edge = current_root->puct_select(c_puct_root);

            while (selected_edge && selected_edge->is_expanded()) {

                next_node = selected_edge->get_node();
                edge_parent->unlock();

                // If a terminal node is hit (actually this shouldn't ever happen), reset the selection
                if (selected_edge->is_terminal()) {
                    edge_parent->unlock();
                    edge_parent = current_root;
                    edge_parent->lock();
                    selected_edge = current_root->puct_select(c_puct_root);
                    continue;
                }


                // If an unevaluated node is hit, the selection has to be stalled until the backend processes it.
                if (!next_node->evaluated)
                    uneval_hit(next_node);

                edge_parent = next_node;
                edge_parent->lock();
                selected_edge = edge_parent->puct_select(c_puct);
            }


            if (selected_edge) {

                //for (selected_edge = edge_parent->begin(); selected_edge != edge_parent->end(); selected_edge++)
                {
                    n_selection_fails = 0;

                    bool res = selected_edge->expand(edge_parent, memory_);
                    auto node = selected_edge->get_node();



                    if (res) {

#ifdef TRANSPOSITION_TABLES_ENABLED
                        auto transposition = memory_.transposition_check(node);

                        if (transposition && transposition->evaluated)
                        {

                            //if (!transposition->evaluated)
                            //    uneval_hit(transposition);

                            transposition->lock();

                            node->Q_ = transposition->Q_;
                            node->moves_left = transposition->moves_left;
                            memcpy(node->begin(), transposition->begin(), node->edge_count * sizeof(mcts::edge));

                            transposition->unlock();

                            node->visit_count = 1;
                            node->visits_pending = 0;
                            node->solution = solution_state::unsolved;

                            node->parent->update_value(-node->Q_);

                            for (auto &i: *node)
                                i.node_ = nullptr;

#ifdef DEBUG_CHECKS
                            node->transposition = true;
#endif
                            node->evaluated = true;
                            node->unlock();
                            num_transpositions++;

                            net_manager.nodes_processed++;
                        } else
#endif
                            add_to_shared_batch(node);
                    } else {
                        //net_manager.blocking_inference(batch);
                        //batch.clear();
                        if (current_root->is_solved()) {
                            nodes_to_expand = 0;
                            paused = true;
                            break;
                        }
                    }
                }
                edge_parent->unlock();
            }
            else
            {
                edge_parent->unlock();

                if (current_root->is_solved() || edge_parent == current_root || ++n_selection_fails == 10) {
                    nodes_to_expand = 0;
                    paused = true;
                    break;
                }
            }
        }


        _break_twice:
        n_selection_fails = 0;
        process_shared_batch();
        working_threads--;
    }
}


vector<mcts::node*>* mcts::search::get_buffer()
{
    if (free_buffers.empty())
    {
        auto new_buffer = new vector<mcts::node*>();
        new_buffer->reserve(net_manager.get_max_batch_size());
        all_buffers.push_back(new_buffer);
        return new_buffer;
    }
    else
    {
        auto buffer = free_buffers.back();
        free_buffers.pop_back();
        return buffer;
    }
}


void mcts::search::add_to_shared_batch(mcts::node *node)
{
    shared_batch_insertion_lock.lock();

    if (shared_batch->size() >= net_manager.get_max_batch_size()) {
        shared_batch_insertion_lock.unlock();
        while (shared_batch->size() >= net_manager.get_max_batch_size()) std::this_thread::yield();
        shared_batch_insertion_lock.lock();
    }
    shared_batch->push_back(node);

    node->batch_pointer = shared_batch;

    shared_batch_insertion_lock.unlock();

    if (shared_batch->size() >= net_manager.get_max_batch_size())
        process_shared_batch();
}

void mcts::search::process_shared_batch()
{
    //if (shared_batch.empty()) return;

    static std::mutex buffers_lock;

    shared_batch_inference_lock.lock();
    if (!shared_batch->empty())
    {
        buffers_lock.lock();
        auto local_buffer = get_buffer();
        buffers_lock.unlock();

        shared_batch_insertion_lock.lock();
        std::swap(shared_batch, local_buffer);
        shared_batch_insertion_lock.unlock();

        shared_batch_inference_lock.unlock();


        net_manager.blocking_inference(*local_buffer);
        batches++;
        local_buffer->clear();

        std::lock_guard l(buffers_lock);
        all_buffers.emplace_back(local_buffer);
    }
    else shared_batch_inference_lock.unlock();
}

void mcts::search::stop_search()
{
    cout << "info stopping search." << endl;
    paused = true;
    while (working_threads);
}

void mcts::search::expand_tree(std::chrono::duration<long, std::ratio<1l, 1000l> > time_left, size_t node_limit)
{
    cout << "info prepare search" << endl;
    if (!prepare_search())
        return;


    auto initial_time_limit = time_left / 20;

    auto start = chrono::high_resolution_clock::now();
    auto end = start + initial_time_limit - 10ms;


    hash_collisions = 0;
    num_transpositions = 0;
    net_manager.time_spent_waiting = 0;
    batches = 0;
    net_manager.reset_nps();
    this->nodes_to_expand = node_limit;

    pausing_mutex.lock();
    paused = false;
    paused_cv.notify_all();
    pausing_mutex.unlock();

    cout << "info unpaused" << endl;


    bool timemod = false;

    int counter = 0;
    do
    {
        if (++counter == 10) {
            net_manager.print_pipeline_information(cout);
            cout << "  |  Solved: " << solved_nodes << "  |  Transpositions: " << num_transpositions <<
            "  |  Average batch size: " << float(net_manager.nodes_processed) / batches << endl;
            counter = 0;
        }
        this_thread::sleep_for(100ms);

        if (std::chrono::high_resolution_clock::now() >= end)
        {
            stop_search();
            break;
        }

        if (!timemod) {
            int most_visited = 0;
            int second_most_visited = 0;
            floatx best_value = -1, second_best_value;
            for (auto &i: *current_root) {
                if (i.get_node()) {
                    //best_value = max(best_value, i.get_node()->average_value());

                    auto val = i.get_value();

                    if (val > best_value) {
                        second_best_value = best_value;
                        best_value = val;
                    } else if (val > second_best_value)
                        second_best_value = val;


                    auto visits = i.get_node()->visit_count;
                    if (visits > most_visited) {
                        second_most_visited = most_visited;
                        most_visited = visits;
                    } else if (visits > second_most_visited) {
                        second_most_visited = visits;
                    }
                }
            }

            if (abs(best_value - second_best_value) < .1) {
                end += initial_time_limit / 2;
                timemod = true;
            } else if (abs(best_value - second_best_value) > .3 && most_visited > 512) {

                end -= initial_time_limit / 2;
                timemod = true;
                //cout << "Aborting.." << endl;
                //stop_search();
                //break;
            }
        }
    }
    while (working_threads);


    process_shared_batch();



    float wait_time = net_manager.time_spent_waiting;
    wait_time /= 1000000;
    //cout << "Wait time sum: " << wait_time << "   total time: " << chrono::duration_cast<chrono::milliseconds>(
    //        chrono::high_resolution_clock::now() - start
    //        ).count() << endl;

    //cout << "info Hash collisions: " << hash_collisions << endl;
    //cout << "info Successful transpositions: " << num_transpositions << endl;
}


bool mcts::search::is_searching() const {
    return working_threads != 0;
}

void mcts::search::uneval_hit(mcts::node *node_)
{
    if (node_->batch_pointer == shared_batch)
        process_shared_batch();

    net_manager.wait_for_node_evaluation(node_);
}






















/*
#ifndef SYNCHRONOUS_INFERENCE
void mcts::search::expand_tree_probabilistic_worker()
{
    int uneval_hit = 0;
    int n_collisions = 0;


    auto timer_start = chrono::high_resolution_clock::now();
    uint64_t select_time = 0;
    auto total_time_start = chrono::high_resolution_clock::now();


    while(nodes_to_expand > 0)
    {
        reselect:
        mcts::node* edge_parent = current_root;
        mcts::edge* selected_edge = current_root->probabilistic_select();



        while (selected_edge && selected_edge->is_expanded())
        {
            // If a terminal node is hit, reset the selection

            if (selected_edge->is_terminal())
            {
                edge_parent = current_root;
                selected_edge = current_root->probabilistic_select();
                continue;
            }
            //timer_start = chrono::high_resolution_clock::now();
            if (!selected_edge->get_node()->evaluated) {

                net_manager.wait_for_node_evaluation(selected_edge->get_node());


            }
            //select_time += chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - timer_start).count();

            edge_parent = selected_edge->get_node();
            //selected_edge = edge_parent->get_edges() + (random64() % edge_parent->edge_count);
            selected_edge = edge_parent->probabilistic_select();
        }


        if (selected_edge && selected_edge->expand(edge_parent, memory_)) {
            net_manager.add_to_eval_queue(selected_edge->get_node());
            nodes_to_expand--;
        }

    }

    cout << "Collisions: " << n_collisions << endl;
    cout << "Thread finished in " << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() -
    total_time_start).count() << "  waiting loop: " << select_time << endl;
    working_threads--;
}

void mcts::search::expand_tree_puct_worker()
{
    auto end_search = std::chrono::high_resolution_clock::now() + 10s;

    mcts::edge* pending_selection = nullptr;
    mcts::node* edge_parent;
    mcts::edge* selected_edge;

    while(nodes_to_expand > 0 && working/* && std::chrono::high_resolution_clock::now() < end_search* /)
    {
        if (pending_selection == nullptr || !pending_selection->get_node()->evaluated) {
            edge_parent = current_root;
            selected_edge = current_root->puct_select(c_puct_root);
        }
        else
        {
            edge_parent = pending_selection->get_node();
            selected_edge = edge_parent->puct_select(c_puct);
            pending_selection = nullptr;
        }

        while (selected_edge && selected_edge->is_expanded())
        {
            // If a terminal node is hit, reset the selection

            while (selected_edge->node_ == (mcts::node*)-1) this_thread::yield();

            if (selected_edge->is_terminal())
            {
                edge_parent = current_root;
                selected_edge = current_root->puct_select(c_puct_root);

                continue;
            }


            // If an unevaluated node is hit, the selection has to be stalled until the backend processes it.
            if (!selected_edge->get_node()->evaluated) {

                /*
                if (!pending_selection) {
                    pending_selection = selected_edge;
                    selected_edge = nullptr;
                    break;
                }
                else
                {
                    if (!pending_selection->get_node()->evaluated)
                        net_manager.wait_for_node_evaluation(pending_selection->get_node());

                    std::swap(pending_selection, selected_edge);
                }
                 * /

                net_manager.wait_for_node_evaluation(selected_edge->get_node());
            }

            edge_parent = selected_edge->get_node();
            selected_edge = edge_parent->puct_select(c_puct);
        }


        if (selected_edge)
        {
            if (selected_edge->expand(edge_parent, memory_))
            {
                auto node = selected_edge->get_node();
                std::lock_guard lock(*node);

                auto transposition = memory_.transposition_check(node);

                if (transposition)
                {
                    num_transpositions++;
                    node->visit_count = 1;
                    node->visits_pending = 0;
                    node->Q_ = transposition->Q_;

                    node->moves_left = transposition->moves_left;
                    node->solution = transposition->solution;


                    node->parent->update_value(-node->Q_);

                    memcpy(node->begin(), transposition->begin(), node->edge_count * sizeof(mcts::edge));

                    for (auto &i: *node)
                        i.node_ = nullptr;


                    node->evaluated = true;

                    net_manager.nodes_processed++;
                }
                else net_manager.add_to_eval_queue(selected_edge->get_node());
            }
            else
            {
                if (current_root->is_solved()) {
                    nodes_to_expand = 0;
                    working_threads--;
                    return;
                }
            }
            nodes_to_expand--;
        }
    }

    working_threads--;
}
#endif

*/

