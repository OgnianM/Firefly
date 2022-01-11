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

#ifndef FIREFLY_MCTS_NODE_H
#define FIREFLY_MCTS_NODE_H

#include <chess/board.h>
#include <atomic>
#include <vector>
#include <mutex>
#include "memory.h"


extern mcts::node* current_root;

extern std::atomic<int> solved_nodes;

/*
 * Explanation of the memory model.
 *
 * When a block of nodes is allocated
 */

namespace mcts {

    enum solution_state : uint8_t
    {
        // Search can proceed normally
        // Q_ holds the approximate value for the current position
        unsolved,

        // Subtree is solved, no additional searching is necessary
        // Q_ ∈ {-1,0,1}
        solved,

        // Subtree is in tablebase, no additional eval necessary if DTZ available
        // Should also be able to use WDL only, in most instances
        // Q_ ∈ {-1,0,1}
        tablebase
    };

    // winning > drawn > losing
    enum game_result
    {
        losing,
        drawn,
        winning,
    };



    template<typename T>
    struct copyable_atomic : public std::atomic<T>
    {
        using std::atomic<T>::atomic;
        using std::atomic<T>::operator=;

        copyable_atomic(const copyable_atomic& other)
        {
            this->store(other.load());
        }
    };
    struct node;


    /*
     * On solving branches:
     * If a branch becomes solved, it should be moved to the end of all edges.
     *
     *
     */
    struct edge
    {
        /*
         * If not expanded, then node == nullptr
         */
        node* node_;


        union {
            float P_;
            float terminal_value;
        };

        chess::move move;
        bool terminal;


        std::string print() const;


        explicit edge(chess::move const& move);
        void set_prior(float new_p)
        {
            P_ = new_p;
        }


        /*
         * 1. Copy parent board.
         * 2. Apply move to board
         * 2a. Check for fifty move rule or threefold repetition
         * 3. Generate moves.
         * 3a. Check for checkmate or stalemate.
         * 4. Allocate node and edges
         *
         * If the state after 2. ends up being terminal for any reason,
         * the edge doesn't allocate a node, instead it just stores the
         * state's value in terminal_value
         */
        bool expand(mcts::node* parent, memory& memory_);

        inline bool is_expanded() const
        {
            return node_ != nullptr || terminal;
        }
        inline bool is_terminal() const
        {
            return terminal;
        }
        void set_terminal(chess::game_state state, mcts::node* parent);

        inline mcts::node* get_node()
        {
            return node_;
        }

        float get_value() const;
    };


    struct prior_iterator : public std::iterator_traits<mcts::edge*>
    {

        prior_iterator(mcts::edge* edge)
        {
            ptr = edge;
        }

        inline float& operator*() const
        {
            return ptr->P_;
        }

        inline void operator++()
        {
            ptr++;
        }

        size_t operator-(mcts::prior_iterator const& other) const
        {
            return ptr - other.ptr;
        }
        inline prior_iterator operator+(size_t n) const
        {
            return {ptr+n};
        }

        bool operator==(prior_iterator const& other)
        {
            return ptr == other.ptr;
        }

        bool operator!=(prior_iterator const& other)
        {
            return ptr != other.ptr;
        }
    private:
        mcts::edge* ptr;
    };



    class node
    {
        edge* tablebase_move();
        edge* best_edge_by_value();

        friend class mcts::edge;
        inline void update_value_for_terminal_child(chess::game_state state, int child_idx)
        {
            //move_to_end(child_idx);
            //update_value(state == chess::checkmate ? -1 : 0);
            adjust_value_for_solved_branch<true>(0, state == chess::game_state::checkmate ? winning : drawn, 0, child_idx);
        }
    public:
        // 8 bytes
        node *parent;

        //region Data
        //34 bytes
        chess::board board;

        vector<mcts::node*>* batch_pointer = nullptr;



        // 4 bytes
        floatx Q_;

        // 8 bytes
        uint32_t visit_count;
        copyable_atomic<uint32_t> visits_pending;


        // 2 bytes
        uint8_t edge_count; // Hope it never needs more than 256 edges
        uint8_t index_in_parent;

        /*
         * Initialized as equal to edge_count on construction.
         * If a branch is terminal, viable_children gets decremented.
         * If all branches are terminal, viable_children == 0 and the
         * current subtree is marked as both terminal (by proxy) and solved
         * and the parent's viable_children gets decremented.
         *
         * If viable_children == 0, then Q_ is set to the actual value of the solved subtree.
         */
        uint8_t viable_edges;

        uint8_t moves_left; // moves_left == min(moves_left, 255)

#ifdef DEBUG_CHECKS // Used to identify nodes created by transposition for debugging purposes
        bool transposition = false;
#endif
        //region Flags
        uint8_t repetitions: 2;
        bool reversible_move: 1;
        bool evaluated:1;
        solution_state solution : 2;
        //endregion

        //endregion

        static thread_local uint8_t thread_id;
        copyable_atomic<uint8_t> locking_tid;
        uint8_t lock_count;

        inline void lock()
        {
            if (locking_tid == thread_id) {
                lock_count++;
                return;
            }
            uint8_t f = -1;
            while (!locking_tid.compare_exchange_weak(f, thread_id)) {
                f = -1;
                std::this_thread::yield();
            }

            lock_count = 1;
        }

        /*
        inline bool try_lock_for(auto duration)
        {
            if (locking_tid == thread_id) return true;

            uint8_t f = -1;
            if (!locking_tid.compare_exchange_weak(f, thread_id))
            {
                auto end = std::chrono::high_resolution_clock::now() + duration;

                f = -1;

                while(!locking_tid.compare_exchange_weak(f,thread_id))
                {
                    if (std::chrono::high_resolution_clock::now() >= end)
                        return false;

                    f = -1;
                }
            }

            return true;
        }*/

        inline void unlock()
        {
#ifdef DEBUG_CHECKS
            if (lock_count == 0)
                throw std::logic_error("Unlocking non-locked node.");
#endif
            if (--lock_count == 0)
                locking_tid = -1;
        }

        /*
        inline void unlock_all()
        {
            lock_count = 0;
            locking_tid = -1;
        }
         */

        inline uint32_t get_n_subnodes() const { return visit_count + visits_pending; }
        inline mcts::edge* get_edges() { return (mcts::edge*)(this+1); }
        inline mcts::edge* get_own_edge() { return (mcts::edge*)(parent + 1) + index_in_parent; }
        inline const mcts::edge* get_own_edge() const { return (mcts::edge*)(parent + 1) + index_in_parent; }

        inline bool is_solved() const { return solution != solution_state::unsolved; }

        inline prior_iterator priors_begin() { return prior_iterator(get_edges()); }
        inline prior_iterator priors_end() { return priors_begin() + edge_count; }

        inline edge *begin() { return get_edges(); }
        inline edge *end() {  return get_edges() + edge_count; }

        inline edge* get_last() { return end() - 1; }

        inline float average_value() const { return Q_; }


        inline edge* best_move()
        {
            switch(solution)
            {
                case solved:
                case unsolved: [[likely]]
                    return best_edge_by_value();

                case tablebase:
                    return tablebase_move();

            }
        }


        inline void make_solved(solution_state state, game_result true_value)
        {

            auto old_Q = Q_;

            switch (true_value)
            {
                case winning:
                    Q_ = 1;
                    break;
                case drawn:
                    Q_ = 0;
                    break;
                case losing:
                    Q_ = -1;
                    break;
            }

            this->evaluated = true;
            this->solution = state;

            if (parent && parent != current_root) {
                parent->lock();
                parent->adjust_value_for_solved_branch(old_Q, true_value, visit_count, index_in_parent);
                parent->unlock();
            }

            //auto& l = logging::log("solver");
            //cout << "info FEN: " << board.to_fen() << " solved as: " << Q_ << endl;

        }



        /*
         * Recursively adjusts the values of a branch which has reached a terminal or solved state.
         * During search, value approximations will be acquired from the NN, if a branch is solved,
         * suddenly fully accurate data is available, so the previous approximations should be
         * adjusted, all the way down to the root.
         *
         * The delta is the difference between the old value and the new value, e.g. if
         * old = .95 and new = 1; delta = .95 - 1 = -.05
         *
         * Weighing the delta means multiplying it by the child's visit count
         */
        inline void propagate_solved_value(float weighted_delta)
        {
            lock();
            auto new_value = (Q_ * visit_count + weighted_delta) / visit_count;

#ifdef DEBUG_CHECKS
            if (new_value > 1.001 || new_value < -1.001)
                throw std::logic_error("Node value out of bounds.");
#endif

            weighted_delta = (Q_ - new_value) * visit_count;
            Q_ = new_value;
            unlock();

            if (parent && parent != current_root)
                parent->propagate_solved_value(weighted_delta);
        }


        /*
         * If a child gets solved, then its value changes and that change should be backpropagated
         * down to the root.
         */
        template<bool FromTerminalChild = false>
        void adjust_value_for_solved_branch(float old_value,
                                               game_result true_value,
                                               uint32_t child_visit_count,
                                               int branch_idx)
        {
            solved_nodes++;
            viable_edges--;

            switch (true_value)
            {
                // If any child is winning, then this node automatically becomes solved as losing
                case game_result::winning:

                    make_solved(solution_state::solved, game_result::losing);
                    break;

                // If the position is drawn, just subtract its previous value from its parent, grandparent, etc.
                // TODO: If a draw is a good result, as when leading in a match, this will be suboptimal
                case game_result::drawn:
                    if (viable_edges == 0)
                    {
                        // If any child was winning, make_solved would've been called before this last branch
                        // got solved, if the child that called this function is the last viable branch,
                        // and it's a draw, then the whole subtree is a draw with best play.
                        make_solved(solution_state::solved, game_result::drawn);
                    }
                    else
                    {

                        // If a true terminal edge called this function, then no adjustment is necessary,
                        // the draw score can be backpropagated directly
                        if constexpr (FromTerminalChild)
                        {
                            update_value(0);
                        }
                        else // If the caller is not a true terminal node
                        {
                            // Adjusting the value for a draw, right now that basically means zeroing out
                            // its influence over its parent, possibly not optimal, see TODO: above
                            auto new_q = (Q_ * visit_count + old_value * child_visit_count) / visit_count;

                            if (parent && parent != current_root)
                                parent->propagate_solved_value((Q_ - new_q) * this->visit_count);
                            Q_ = new_q;
                        }
                    }
                    break;
                case game_result::losing:
                    if (viable_edges == 0)
                    {

                        float val = -1;
                        for (auto& i : *this)
                        {
                            // If the last viable branch is losing, but there is at least one other branch
                            // that leads to a draw, then the subtree is a draw with best play.
                            if (i.get_value() == 0)
                            {
                                //cout << "info last losing branch max_value: 0" << endl;
                                make_solved(solution_state::solved, game_result::drawn);
                                return;
                            }

                            if (i.get_value() > val)
                                val = i.get_value();
                        }

                        //cout << "info last losing branch max_value: " << val << endl;

                        // If every branch from this node is losing, then this node is solved as winning
                        make_solved(solution_state::solved, game_result::winning);
                    }
                    else
                    {
                        // Adjusting the value for a losing child is equivalent to subtracting its
                        // previous value and adding its visit count (i.e. visit_count * 1) to the running average.
                        auto new_q = (Q_ * visit_count + (old_value + 1) * child_visit_count) / visit_count;

#ifdef DEBUG_CHECKS
                        if (new_q > 1.001 || new_q < -1.001)
                            throw std::logic_error("Node value out of bounds.");
#endif
                        if (parent && parent != current_root)
                            parent->propagate_solved_value((Q_ - new_q) * this->visit_count);

                        Q_ = new_q;
                    }
                    break;
            }
            move_to_end(branch_idx);
        }


        void update_value(float value);


        // Put an edge at the end of the edge list, preserving the order of all other edges.
        inline void move_to_end(int edge_idx)
        {
            if (edge_count == 0) return;
            auto branch = begin() + edge_idx;
            auto last = end() - 1;
            if (branch != last)
            {
                auto copy = *branch;

                // If not last branch, move all other branches forward
                for (auto it = branch + 1; it != end(); it++) {

                    if (it->get_node())
                        it->get_node()->index_in_parent--;

                    *(it - 1) = *it;
                }
                *last = copy;
                if(last->get_node())
                    last->get_node()->index_in_parent = last - begin();
            }
        }

        // Sorts edges in ascending order
        inline void sort_edges_by_priors() {
            std::sort(begin(), end(), [](const mcts::edge& a, const mcts::edge& b){
                return a.P_ > b.P_;
            });
        }

        /*
         * Selects an edge according to a probability distribution weighted by the move priors
         */
        mcts::edge* probabilistic_select();

        mcts::edge* puct_select(const float c_puct);


        node(const chess::board &board,
             const chess::movegen_result& moves,
             node *parent,
             bool reversible_move=false,
             uint8_t repetitions=0);


        void generate_children();


        inline const size_t get_total_size() const
        {
            return sizeof(mcts::node) + sizeof(mcts::edge) * edge_count;
        }

        inline void copy_to(void* memory)
        {
            memcpy(memory, this, get_total_size());
            get_own_edge()->node_ = static_cast<mcts::node *>(memory);
            for (auto& i : *this)
            {
                if (i.get_node())
                {
                    i.get_node()->parent = static_cast<mcts::node *>(memory);
                }
            }
        }


    };

}
#endif //FIREFLY_MCTS_NODE_H
