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

//#include <cmath>
//#include "chess/board.h"
//#include "testing/chess_gui.h"
#include <fstream>
#include <external/SenjoUCIAdapter/senjo/UCIAdapter.h>
#include <external/SenjoUCIAdapter/senjo/Output.h>
#include "engine/engine_interface.h"
#include <engine/mcts/search.h>
#include <cxxopts.hpp>
#include <filesystem>
#include <utils/logger.h>

namespace fs = std::filesystem;
using namespace std;


string neural_net_path;


void start_engine(cxxopts::ParseResult& options)
{
    engine_interface interface(options);
    senjo::UCIAdapter adapter(interface);

    std::string line;
    line.reserve(16384);


    cout << "info Engine started..." << endl;

    while (std::getline(std::cin, line)) {

        if (!adapter.doCommand(line))
            break;
    }

}


void benchmark_loop()
{
    torch::InferenceMode imode;
    lc0::LC0Network net("../weights/Leela/384x30-2021_1106_1405_42_923.pb", torch::kCUDA, true);

    float n = 0;
    auto start = chrono::high_resolution_clock::now();
    while (true)
    {
        for (int i = 0; i < 5; i++)
            net->forward(torch::randn({1024, 112, 8, 8}, torch::TensorOptions(torch::kCUDA).dtype(torch::kF16)));

        n += 1024 * 5;

        cout << "NPS: " << 1000 * n / chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count() << endl;

    }
}

void stress_test(cxxopts::ParseResult& init_opts, string position)
{
    mcts::search s(init_opts);
    s.initialize(position);

    s.expand_tree(chrono::years(1));

    cout << s.best_move()->move.to_uci_move() << endl;

}

int main(int argc, char* argv[])
{
    //at::globalContext().setBenchmarkCuDNN(true);
    //benchmark_loop();

    mcts::node::thread_id = -1;
    cxxopts::Options options("Firefly", "Chess engine.");

    options.add_options()
            ("h,help", "This message.")
            ("debug", "Enable debug messages.", cxxopts::value<bool>()->default_value("false"))
            //("tablebases", "Path to syzygy tablebases. (not functional)", cxxopts::value<string>()->default_value("none"))
            ("n,network", "Path to neural network weights.", cxxopts::value<string>())
            ("c,c_puct","CPuct value for non-root nodes.", cxxopts::value<float>()->default_value("1.2"))
            ("c_puct_root", "CPuct value for root nodes.", cxxopts::value<float>()->default_value("2"))
            ("s,softmax_temperature","Softmax temperature for policy.", cxxopts::value<float>()->default_value("1"))
            ("t,threads", "Number of tree traversal threads.", cxxopts::value<int>()->default_value("1"))
            ("d,device", "[auto/cpu/cuda] or [0,1,2,...]\n"
                          "--device=cuda defaults to using all available cuda backends\n"
                          "--device=0,1,... is used to specify exactly which cuda backends should be used, for example "
                          "GPU:0 and GPU:2, but not GPU:1.\n"
                          "--device=cpu uses the cpu\n"
                          "--device=auto - cuda if available, otherwise cpu", cxxopts::value<string>()->default_value("auto"))
            ("cpu_inference_threads", "For use with -d cpu, defaults to half the system threads", cxxopts::value<int>()->default_value("-1"))
            ("deallocation_factor", "Deallocate nodes in bulk only when dead nodes outnumber useful nodes by "
                                "at least deallocation-factor - low values sacrifice CPU time for memory "
                                "efficiency, high values do the opposite.", cxxopts::value<int>()->default_value("32"))
            ("deallocation_minimum", "Minimum number of nodes to deallocate.", cxxopts::value<int>()->default_value("65536"))
            ("dirichlet_epsilon", "", cxxopts::value<float>()->default_value("0"))
            ("dirichlet_alpha", "", cxxopts::value<float>()->default_value("1"))
            ("max_batch_size", "Maximum batch size for NN, high values may cause an OOM error.",
                    cxxopts::value<int>()->default_value("1024"))
            ("graph_log_file", "Log for graphviz logging of the search tree.", cxxopts::value<std::string>()->default_value("none"))
            ("general_log_file", "File for general logging.", cxxopts::value<std::string>()->default_value("none"));

    options.set_width(120);

    cxxopts::ParseResult result = options.parse(argc, argv);

    //stress_test(result, "r4k1r/4bp2/pqppbp2/5p2/4P2p/1BN4P/PPP1Q1P1/1K1R1R2 b - - 3 18");

    if (result["h"].count() > 0)
    {
        cout << options.help() << endl;
        return 0;
    }
    auto graph_log = result["graph_log_file"].as<string>();
    auto default_log = result["general_log_file"].as<string>();

    if (graph_log != "none") {
        logging::init_log_writer("graph", graph_log);
        logging::log("graph") << "digraph mcts_graph { \n";
    }

    if (default_log != "none")
        logging::init_log_writer("default", graph_log);





    /*
    auto tablebase_path = result["tablebases"].as<string>();
    bool tablebases_loaded = false;
    if (tablebase_path != "none")
    {
        tablebases_loaded = tb_init(tablebase_path.c_str());
        if (tablebases_loaded)
            cout << "info Tablebases loaded!" << endl;
        else cout << "info Failed loading tablebases." << endl;
    }
     */


    if (result["n"].count() == 0)
    {
        cout << "No weights file specified." << endl;
        return 0;
    }

    neural_net_path = result["n"].as<string>();



    if (!fs::exists(neural_net_path))
    {
        cout << neural_net_path << " does not exist." << endl;
        return 1;
    }


    start_engine(result);

    if (graph_log != "none")
    {
        logging::log("graph") << "\n}\n";
    }

    logging::flush_all();
    /*
    if (tablebases_loaded)
        tb_free();
    */

     cout << "Exiting." << endl;
     return 0;
}