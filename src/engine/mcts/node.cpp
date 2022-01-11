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


#include "node.h"
#include <cmath>
#include <queue>
#include <random>
#include <mutex>
#include <utils/utils.h>

using namespace std;
using namespace mcts;

thread_local uint8_t mcts::node::thread_id;

//region Edge methods


std::string edge::print() const
{
    stringstream ss;
    if (!terminal)
        ss << "Node move: " << move.to_uci_move() << "  Terminal: false   Prior: " << P_ << "   Node ptr: " << node_;
    else
        ss << "Node move: " << move.to_uci_move() << "  Terminal: true   Value: " << terminal_value;

    return ss.str();
}

mcts::edge::edge(const chess::move &move) : move(move), node_(nullptr), P_(0), terminal(false){}

bool mcts::edge::expand(mcts::node* parent, memory& memory_)
{
    // Just in case two threads reach the same leaf at the same time.

    std::lock_guard lock(*parent);

    if (node_ != nullptr)
    {

        // Decrement pending visits, as this visit will be aborted.
        auto node = parent;
        while (node && node != current_root)
        {
            node->visits_pending--;
            node = node->parent;
        }

        return false;
    }

    // Set node_ to a temporary value, simply to prevent any other threads from trying to expand it

    auto board = parent->board;
    board.make_move(move);

    bool reversible_move = true;
    // Fifty move rule
    if (board.halfmove_clock >= 50)
    {
        set_terminal(chess::game_state::draw, parent);
        return false;
    }
    uint8_t repetitions = 0;

    if (board.halfmove_clock == 0) {
        reversible_move = false;
    }
    else [[likely]]
    {
        // Check for threefold repetition.
        auto node = parent;

        while (node && node->reversible_move)
        {
            if (board.tfr_compare(node->board)) {
                repetitions = node->repetitions + 1;
                if (repetitions == 3) {
                    //set_terminal(chess::game_state::draw, parent);
                    //return false;
                }
                break;
            }
            node = node->parent;
        }

        reversible_move = true;
    }

    board.has_repeated = repetitions != 0;

    chess::movegen_result moves;
    auto gstate = board.generate_moves(moves);

    if (gstate != chess::game_state::playing)
    {
        set_terminal(gstate, parent);
        return false;
    }

    node_ = new (memory_.allocate_fused_node(moves.moves_count))
            mcts::node(board, moves, parent, reversible_move, repetitions);

    node_->index_in_parent = this - parent->get_edges();

    return true;
}

void mcts::edge::set_terminal(chess::game_state state, node *parent) {
    terminal_value = state == chess::game_state::checkmate ? 1 : 0;
    terminal = true;
    node_ = nullptr;
    //parent->update_value(-terminal_value);
    parent->update_value_for_terminal_child(state, this - parent->begin());
}

float mcts::edge::get_value() const {
    // If the node is not expanded, assume it's losing
    return is_expanded() ? (is_terminal() ? terminal_value : this->node_->average_value()) : -1;
}
//endregion


// region Constructors
mcts::node::node(const chess::board &board,
                 const chess::movegen_result& moves,
                 node* parent,
                 bool reversible_move,
                 uint8_t repetitions) :
    board(board),
    visit_count(0),
    Q_(0),
    parent(parent),
    edge_count(moves.moves_count),
    viable_edges(moves.moves_count),
    visits_pending(1),
    reversible_move(reversible_move),
    repetitions(repetitions),
    evaluated(false),
    solution(unsolved),
    locking_tid(thread_id),
    lock_count(1)
{
    auto edges = get_edges();
    for (int i = 0; i < edge_count; i++)
        new (edges + i) mcts::edge(moves.moves[i]);

    /*
     * The board is now seen from the player to move's perspective, however the
     * node's value is from the perspective of the player who just moved to get to this state,
     * that's the reason for the counterintuitive tablebase_win -> game_result::losing and vice versa
     */
    /*
    switch(board.tb_probe_wdl())
    {
        case chess::game_state::tablebase_win:
            make_solved(solution_state::tablebase, game_result::losing);
            break;

        case chess::game_state::tablebase_draw:
            make_solved(solution_state::tablebase, game_result::drawn);
            break;

        case chess::game_state::tablebase_loss:
            make_solved(solution_state::tablebase, game_result::winning);
            break;
    }
     */
}


//endregion



void mcts::node::update_value(float value)
{
#ifdef DEBUG_CHECKS
    if (Q_ > 1.001 || Q_ < -1.001)
        throw std::logic_error("Node value out of bounds.");
#endif
    lock();
    Q_ = (Q_ * visit_count + value) / ++visit_count;

    visits_pending--;
    unlock();


    if (parent && parent != current_root)
        parent->update_value(-value);
    /*
    auto node = parent;

    while (node && node != current_root)
    {
        value = -value;

        node->lock();

        node->Q_ = (node->Q_ * node->visit_count + value) / ++node->visit_count;
        node->visits_pending--;

        node->unlock();

        node = node->parent;
    }
    */

}


mcts::edge* mcts::node::probabilistic_select() {

    std::discrete_distribution<> dist(priors_begin(), priors_end());

    return &get_edges()[dist(get_rng())];
}


mcts::edge* mcts::node::puct_select(const float c_puct)
{
    auto pending = visits_pending++;

    //for (auto& i : *this)
    //    if (!i.is_expanded()) return &i;


    if (!begin()->is_expanded()) return begin();

    mcts::edge* best_node = nullptr;
    floatx best_score = -std::numeric_limits<float>::infinity();

    floatx visits_sqrt = sqrt(visit_count + pending);

    floatx score = 0;

    for (auto& i : *this)
    {
        if (i.is_expanded())
        {
            if (i.is_terminal() || i.get_node()->is_solved()) [[unlikely]] {
                return best_node;
            }

            //bool locked = i.get_node()->try_lock_for(1ms);

            score = i.get_node()->average_value() + i.P_ *
                    (visits_sqrt / (i.get_node()->visit_count + i.get_node()->visits_pending + 1)) * c_puct;

            //if(locked) i.get_node()->unlock();

            if (score > best_score)
            {
                best_node = &i;
                best_score = score;
            }
        }
        else if ((i.P_ * c_puct * visits_sqrt) > best_score)
            return &i;
    }

    return best_node;
}
