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


#include "engine_interface.h"
#include <external/SenjoUCIAdapter/senjo/Output.h>
#include <testing/chess_gui.h>

using namespace std;

extern std::string neural_net_path;

std::string engine_interface::getAuthorName() const {
    return "Ognyan Mirev";
}

std::string engine_interface::getEngineName() const {
    return "Firefly";
}

std::string engine_interface::getEngineVersion() const {
    return "1.0.0";
}

engine_interface::engine_interface(cxxopts::ParseResult& options) : search(options)
{

    //search.net_manager = new network_manager<lc0::LC0Network>(1,1024);
    //search.net_manager->autodetect_backends(neural_net_path);

    search.initialize("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    debug = false;
    senjo::EngineOption limit_strength;
    limit_strength.setName("UCI_LimitStrength");
    limit_strength.setValue(0);
    engine_options.push_back(limit_strength);
}

std::list<senjo::EngineOption> engine_interface::getOptions() const {
    return this->engine_options;
}

bool engine_interface::setEngineOption(const std::string &optionName, const std::string &optionValue) {

    senjo::Output() << "Set: " << optionName << " = " << optionValue << '\n';
    return true;
}



void engine_interface::initialize() {
}

bool engine_interface::isInitialized() const {
    return true;
}



bool engine_interface::setPosition(const std::string &fen, std::string *remain) {
    auto result = search.initialize(fen);
    return result;
}

bool engine_interface::makeMove(const std::string &move)
{
    auto result = search.make_move_external(move);
    return result;
}

std::string engine_interface::getFEN() const {
    return search.current_root->board.to_fen();
}

void engine_interface::printBoard() const {
    search.current_root->board.print();
}

bool engine_interface::whiteToMove() const {
    return !search.current_root->board.flipped;
}

void engine_interface::clearSearchData() {
}

void engine_interface::ponderHit() {

}

void engine_interface::setDebug(const bool flag) {
    debug = flag;
}

bool engine_interface::isDebugOn() const {
    return debug;
}

bool engine_interface::isSearching() {
    return search.is_searching();
}

void engine_interface::stopSearching() {
    search.stop_search();
}

bool engine_interface::stopRequested() const {
    return false;
}

void engine_interface::waitForSearchFinish() {
    while(search.is_searching()) std::this_thread::sleep_for(5ms);
}

uint64_t engine_interface::perft(const int depth)
{
    uint64_t sum = 0;
    for (auto& i : search.root_board.perft(depth)) sum += i.second;
    return sum;
}

std::string engine_interface::go(const senjo::GoParams &params, std::string *ponder)
{
    auto my_time = search.current_root->board.flipped ?params.btime : params.wtime;

    // Only try to do a free operation if there are more than 10 seconds on the clock
    if (my_time > 10000)
        search.free_memory();

    if (search.game_has_ended)
    {
        cout << "info ERROR: search requested but the game has ended, generating new tree without draw states.";
        auto new_board = search.current_root->board;
        new_board.halfmove_clock = 0;

        search.initialize(new_board.to_fen());
    }

    if (params.infinite)
        search.expand_tree(std::chrono::years(69), -1);
    else
        search.expand_tree(std::chrono::milliseconds(my_time), -1);


    auto best_move = search.best_move();

    cout << "Selecting: " << best_move->move.to_uci_move() << " : " << best_move->get_value() << endl;

    return best_move->move.to_uci_move();
}

senjo::SearchStats engine_interface::getSearchStats() const {

    senjo::SearchStats result;

    result.depth = 1;
    result.nodes = search.net_manager.nodes_processed;
    return result;
}