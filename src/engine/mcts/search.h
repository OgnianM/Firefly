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

#ifndef FIREFLY_SEARCH_H
#define FIREFLY_SEARCH_H

#include "node.h"
#include <engine/neural/lc0_network.h>
#include <thread>
#include <engine/neural/network_manager.h>
#include <engine/mcts/memory.h>
#include <cxxopts.hpp>

namespace mcts {

    struct search {
        node *current_root;
        std::vector<std::unique_ptr<mcts::node>> past_roots;
        network_manager<lc0::LC0Network> net_manager;

        std::vector<chess::move> moves_played;

        float dirichlet_alpha, dirichlet_epsilon;

        float c_puct_root, c_puct;

        chess::board root_board;
        bool game_has_ended = false;

        search(cxxopts::ParseResult&);
        ~search();
        bool initialize(string const& fen);

        bool make_move_external(string const& uci_move);


        mcts::edge* best_move() const;


        void advance_root(mcts::edge * edge_to_new_root);



        /*
         * Returns true if the tree should be expanded, false otherwise
         * If it returns false, call best_move directly
         */

        bool prepare_search();

        void expand_tree(std::chrono::duration<long, std::ratio<1l, 1000l> > time_left, size_t node_limit=-1);

        void stop_search();

        //endregion
        void free_memory();

        bool is_searching() const;
    private:

        vector<mcts::node*>* get_buffer();


        void uneval_hit(mcts::node* node_);

        void process_shared_batch();
        void add_to_shared_batch(mcts::node* node);

        memory memory_;
        std::atomic<size_t> working_threads;
        void expand_tree_puct_worker_synchronous();


        //void expand_tree_probabilistic_worker();
        //void expand_tree_puct_worker();

        std::vector<std::jthread> threads;
        size_t thread_count;

        std::atomic<int> nodes_to_expand;

        std::atomic<bool> abort_expansion;

        size_t approximate_nodes_to_clear = 0;

        int deallocation_factor = 32;
        int deallocation_minimum = 65536;
        bool working;


        bool paused = true;
        std::mutex pausing_mutex;
        std::condition_variable paused_cv;

        std::mutex shared_batch_inference_lock, shared_batch_insertion_lock;
        vector<mcts::node*>* shared_batch;

        std::vector<vector<mcts::node*>*> free_buffers, all_buffers;
    };

};
#endif //FIREFLY_SEARCH_H
