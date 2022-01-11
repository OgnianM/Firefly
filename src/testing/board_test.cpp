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

#include "board_test.h"
#include <utils/utils.h>
#include "chess_gui.h"
#include <chrono>
#include <sstream>
#include <unordered_map>

void board_stress_test()
{
    chess::board board, backup;

    chess::movegen_result moves;
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes = 0;
    while (true) {
        board.set_default_position();

        while (board.generate_moves(moves) == chess::playing) {
            //cout << "Backup fen: " << backup.to_fen() << endl;

            board.make_move(moves.moves[random64() % moves.moves_count]);
            nodes++;
            //  backup = board;
        }

        if (nodes > 1000000)
        {
            cout << "1000000 nodes calculated in " << std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start
                    ).count() << "ms" << endl;
            nodes = 0;
            start = std::chrono::high_resolution_clock::now();
        }
    }

}


void test_pos(string fen)
{
    chess::board b;
    if (!b.from_fen(fen))
    {
        cout << "Invalid FEN." << endl;
        return;
    }


    BoardGUI gui;

    while (true)
    {
        gui.Update(b);
        b.make_move(gui.GetMove(b));
    }
}

void test_pos(chess::board b)
{
    BoardGUI gui;

    while (true)
    {
        gui.Update(b);
        b.make_move(gui.GetMove(b));
    }
}

void fen_perft(string fen, int depth, string expected_str)
{
    std::stringstream ss(expected_str);
    string line;

    unordered_map<string, uint64_t> expected_values;

    while (getline(ss,line, '\n'))
    {
        auto cpos = line.find(':');
        string move = line.substr(0, cpos);
        string node_count = line.substr(cpos+1);
        expected_values[move] = stoull(node_count);
    }

    chess::board board;
    board.from_fen(fen);

    cout << "Perft for FEN: " << fen << "\nDepth: " << depth << endl;

    auto start = std::chrono::high_resolution_clock::now();
    auto perft_result = board.perft(depth);

    auto exec_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();


    uint64_t total_nodes = 0;

    for (auto& z : expected_values)
    {
        total_nodes += perft_result[z.first];
        cout << z.first << ": " << perft_result[z.first];
        if (perft_result[z.first] != z.second)
            cout << " mismatched, expected: " << z.second;


        cout << endl;
    }
    cout << '\n' << "Total nodes: " << total_nodes << " computed in " << exec_time << "ms." << endl;
}

int mai00n()
{
    //print_bitboard(9259542123273814144);
    //print_bitboard(72340172838076673);
    //return 0;

    fen_perft("rnb1kbnr/pppp1ppp/4pq2/8/5P2/8/PPPPPKPP/RNBQ1BNR w kq - 2 3", 3,
                     "a2a3: 746\n"
                     "b2b3: 829\n"
                     "c2c3: 786\n"
                     "d2d3: 873\n"
                     "e2e3: 1123\n"
                     "g2g3: 832\n"
                     "h2h3: 742\n"
                     "f4f5: 756\n"
                     "a2a4: 808\n"
                     "b2b4: 808\n"
                     "c2c4: 841\n"
                     "d2d4: 877\n"
                     "e2e4: 1097\n"
                     "g2g4: 843\n"
                     "h2h4: 808\n"
                     "b1a3: 773\n"
                     "b1c3: 810\n"
                     "g1f3: 957\n"
                     "g1h3: 776\n"
                     "d1e1: 739\n"
                     "f2e1: 692\n"
                     "f2e3: 697\n"
                     "f2f3: 801\n"
                     "f2g3: 696");
    /*

    fen_perft("rnbqkbnr/pppp1ppp/4p3/8/5P2/8/PPPPPKPP/RNBQ1BNR b kq - 1 2", 4, "e6e5: 18921\n"
                     "a7a6: 16733\n"
                     "b7b6: 17908\n"
                     "c7c6: 17558\n"
                     "d7d6: 17196\n"
                     "f7f6: 15407\n"
                     "g7g6: 17997\n"
                     "h7h6: 16735\n"
                     "a7a5: 17929\n"
                     "b7b5: 17889\n"
                     "c7c5: 17195\n"
                     "d7d5: 19721\n"
                     "f7f5: 15467\n"
                     "g7g5: 18900\n"
                     "h7h5: 17847\n"
                     "b8a6: 17319\n"
                     "b8c6: 19046\n"
                     "g8f6: 15555\n"
                     "g8h6: 16116\n"
                     "g8e7: 13925\n"
                     "f8a3: 16924\n"
                     "f8b4: 18293\n"
                     "f8c5: 3927\n"
                     "f8d6: 17871\n"
                     "f8e7: 16163\n"
                     "d8h4: 2312\n"
                     "d8g5: 22694\n"
                     "d8f6: 19710\n"
                     "d8e7: 16043\n"
                     "e8e7: 14055");

    fen_perft("rnbqkbnr/pppp1ppp/4p3/8/5P2/8/PPPPP1PP/RNBQKBNR w KQkq - 0 2", 5, "a2a3: 352567\n"
                     "b2b3: 410571\n"
                     "c2c3: 429943\n"
                     "d2d3: 498398\n"
                     "e2e3: 782338\n"
                     "g2g3: 439776\n"
                     "h2h3: 346094\n"
                     "f4f5: 403734\n"
                     "a2a4: 419582\n"
                     "b2b4: 392248\n"
                     "c2c4: 457758\n"
                     "d2d4: 556289\n"
                     "e2e4: 782410\n"
                     "g2g4: 407571\n"
                     "h2h4: 421435\n"
                     "b1a3: 378504\n"
                     "b1c3: 451373\n"
                     "g1f3: 515387\n"
                     "g1h3: 376864\n"
                     "e1f2: 493356");
    /*

return 0;

fen_perft("rnbqkbnr/pppppppp/8/8/5P2/8/PPPPP1PP/RNBQKBNR b KQkq - 0 1", 6, "a7a6: 4471037\n"
                 "b7b6: 5318649\n"
                 "c7c6: 5390367\n"
                 "d7d6: 8004575\n"
                 "e7e6: 9316198\n"
                 "f7f6: 4405212\n"
                 "g7g6: 5391730\n"
                 "h7h6: 4468494\n"
                 "a7a5: 5371969\n"
                 "b7b5: 5301712\n"
                 "c7c5: 5869225\n"
                 "d7d5: 8799943\n"
                 "e7e5: 10743677\n"
                 "f7f5: 4076693\n"
                 "g7g5: 6199585\n"
                 "h7h5: 5390507\n"
                 "b8a6: 4865244\n"
                 "b8c6: 5738193\n"
                 "g8f6: 5680287\n"
                 "g8h6: 4811544");

fen_perft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, "a2a3: 106743106\n"
                                                                         "b2b3: 133233975\n"
                                                                         "c2c3: 144074944\n"
                                                                         "d2d3: 227598692\n"
                                                                         "e2e3: 306138410\n"
                                                                         "f2f3: 102021008\n"
                                                                         "g2g3: 135987651\n"
                                                                         "h2h3: 106678423\n"
                                                                         "a2a4: 137077337\n"
                                                                         "b2b4: 134087476\n"
                                                                         "c2c4: 157756443\n"
                                                                         "d2d4: 269605599\n"
                                                                         "e2e4: 309478263\n"
                                                                         "f2f4: 119614841\n"
                                                                         "g2g4: 130293018\n"
                                                                         "h2h4: 138495290\n"
                                                                         "b1a3: 120142144\n"
                                                                         "b1c3: 148527161\n"
                                                                         "g1f3: 147678554\n"
                                                                         "g1h3: 120669525");
*/
    /*

fen_perft("r3k2r/pppq1pbp/2npbnp1/3Pp3/1P2P3/2NQBNP1/P1P2PBP/R3K2R b KQkq - 0 0",5, "g6g5: 3049291\n"
         "a7a6: 3310071\n"
         "b7b6: 2895886\n"
         "h7h6: 3156779\n"
         "a7a5: 3591582\n"
         "b7b5: 2643927\n"
         "h7h5: 3318769\n"
         "c6b4: 3568573\n"
         "c6d4: 3243314\n"
         "c6a5: 3313126\n"
         "c6e7: 3326514\n"
         "c6b8: 2607018\n"
         "c6d8: 2464891\n"
         "f6e4: 4122773\n"
         "f6g4: 3181771\n"
         "f6d5: 3904603\n"
         "f6h5: 3158432\n"
         "f6g8: 2800669\n"
         "e6h3: 3210632\n"
         "e6g4: 3220975\n"
         "e6d5: 3991421\n"
         "e6f5: 3305260\n"
         "g7h6: 3275892\n"
         "g7f8: 2829099\n"
         "a8b8: 2886295\n"
         "a8c8: 2849219\n"
         "a8d8: 2592939\n"
         "h8f8: 2840069\n"
         "h8g8: 3006344\n"
         "d7e7: 3394851\n"
         "d7c8: 3069792\n"
         "d7d8: 3243081\n"
         "e8e7: 3818306\n"
         "e8d8: 2855379\n"
         "e8f8: 2999602\n"
         "e8g8: 3120184\n"
         "e8c8: 2527354");
//return 0;

fen_perft("r3k2r/pppq1pbp/3pbnp1/3Pp3/1P1nP3/2NQBNP1/P1P2PBP/R3K2R w KQkq - 1 0", 4, "a2a3: 78902\n"
             "h2h3: 77434\n"
             "g3g4: 73875\n"
             "b4b5: 66606\n"
             "a2a4: 79101\n"
             "h2h4: 79021\n"
             "d5e6: 78127\n"
             "c3b1: 75484\n"
             "c3d1: 73630\n"
             "c3e2: 81923\n"
             "c3a4: 82237\n"
             "c3b5: 73032\n"
             "f3g1: 72232\n"
             "f3d2: 72269\n"
             "f3d4: 63625\n"
             "f3h4: 75510\n"
             "f3e5: 87381\n"
             "f3g5: 73882\n"
             "g2f1: 71900\n"
             "g2h3: 78327\n"
             "e3c1: 75207\n"
             "e3d2: 69875\n"
             "e3d4: 58811\n"
             "e3f4: 81992\n"
             "e3g5: 74348\n"
             "e3h6: 71361\n"
             "a1b1: 77286\n"
             "a1c1: 73795\n"
             "a1d1: 71906\n"
             "h1f1: 71907\n"
             "h1g1: 75485\n"
             "d3d1: 67112\n"
             "d3f1: 66630\n"
             "d3d2: 65351\n"
             "d3e2: 73467\n"
             "d3c4: 79776\n"
             "d3d4: 58687\n"
             "d3b5: 63746\n"
             "d3a6: 80070\n"
             "e1d1: 74732\n"
             "e1f1: 73196\n"
             "e1d2: 77074\n"
             "e1c1: 73387\n"
             "e1g1: 73615");

    fen_perft("rnbqkbnr/pppppppp/8/8/6P1/8/PPPPPP1P/RNBQKBNR b KQkq - 0 0", 6, "a7a6: 4882297\n"
                     "b7b6: 5834415\n"
                     "c7c6: 5906478\n"
                     "d7d6: 8399488\n"
                     "e7e6: 10551117\n"
                     "f7f6: 4886646\n"
                     "g7g6: 5821884\n"
                     "h7h6: 4926876\n"
                     "a7a5: 5869664\n"
                     "b7b5: 5825229\n"
                     "c7c5: 6427261\n"
                     "d7d5: 9187585\n"
                     "e7e5: 10589861\n"
                     "f7f5: 6295816\n"
                     "g7g5: 4784860\n"
                     "h7h5: 6939515\n"
                     "b8a6: 5317188\n"
                     "b8c6: 6227415\n"
                     "g8f6: 6267905\n"
                     "g8h6: 5351518");


    fen_perft("rnbqkbnr/1ppppppp/8/p7/6P1/8/PPPPPP1P/RNBQKBNR w KQkq - 0 1",
              5, "a2a3: 216312\n"
                     "b2b3: 258135\n"
                     "c2c3: 262952\n"
                     "d2d3: 386985\n"
                     "e2e3: 404944\n"
                     "f2f3: 214460\n"
                     "h2h3: 201498\n"
                     "g4g5: 221425\n"
                     "a2a4: 221588\n"
                     "b2b4: 295967\n"
                     "c2c4: 283882\n"
                     "d2d4: 424795\n"
                     "e2e4: 407898\n"
                     "f2f4: 237279\n"
                     "h2h4: 261162\n"
                     "b1a3: 238650\n"
                     "b1c3: 278299\n"
                     "g1f3: 280289\n"
                     "g1h3: 223004\n"
                     "f1g2: 350676\n"
                     "f1h3: 199464");

    fen_perft("rnbqkbnr/pppppppp/8/8/5P2/8/PPPPP1PP/RNBQKBNR w KQkq - 0 1", 6, "a2a3: 4488103\n"
                     "b2b3: 5335141\n"
                     "c2c3: 5415707\n"
                     "d2d3: 6651933\n"
                     "e2e3: 9787437\n"
                     "g2g3: 5399790\n"
                     "h2h3: 4487890\n"
                     "f4f5: 4388602\n"
                     "a2a4: 5388273\n"
                     "b2b4: 5315720\n"
                     "c2c4: 5892194\n"
                     "d2d4: 7392277\n"
                     "e2e4: 9825093\n"
                     "g2g4: 5275398\n"
                     "h2h4: 5409665\n"
                     "b1a3: 4881550\n"
                     "b1c3: 5732255\n"
                     "g1f3: 6619668\n"
                     "g1h3: 4848720\n"
                     "e1f2: 6597796");


    fen_perft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, "a2a3: 4463267\n"
                                                                             "b2b3: 5310358\n"
                                                                             "c2c3: 5417640\n"
                                                                             "d2d3: 8073082\n"
                                                                             "e2e3: 9726018\n"
                                                                             "f2f3: 4404141\n"
                                                                             "g2g3: 5346260\n"
                                                                             "h2h3: 4463070\n"
                                                                             "a2a4: 5363555\n"
                                                                             "b2b4: 5293555\n"
                                                                             "c2c4: 5866666\n"
                                                                             "d2d4: 8879566\n"
                                                                             "e2e4: 9771632\n"
                                                                             "f2f4: 4890429\n"
                                                                             "g2g4: 5239875\n"
                                                                             "h2h4: 5385554\n"
                                                                             "b1a3: 4856835\n"
                                                                             "b1c3: 5708064\n"
                                                                             "g1f3: 5723523\n"
                                                                             "g1h3: 4877234");

    return 0;
    fen_perft("r3k2r/pppq1pbp/2npbnp1/3Pp3/1P2P3/2NQBNP1/P1P2PBP/R3K2R b KQkq - 0 0",5, "g6g5: 3049291\n"
                                                                                        "a7a6: 3310071\n"
                                                                                        "b7b6: 2895886\n"
                                                                                        "h7h6: 3156779\n"
                                                                                        "a7a5: 3591582\n"
                                                                                        "b7b5: 2643927\n"
                                                                                        "h7h5: 3318769\n"
                                                                                        "c6b4: 3568573\n"
                                                                                        "c6d4: 3243314\n"
                                                                                        "c6a5: 3313126\n"
                                                                                        "c6e7: 3326514\n"
                                                                                        "c6b8: 2607018\n"
                                                                                        "c6d8: 2464891\n"
                                                                                        "f6e4: 4122773\n"
                                                                                        "f6g4: 3181771\n"
                                                                                        "f6d5: 3904603\n"
                                                                                        "f6h5: 3158432\n"
                                                                                        "f6g8: 2800669\n"
                                                                                        "e6h3: 3210632\n"
                                                                                        "e6g4: 3220975\n"
                                                                                        "e6d5: 3991421\n"
                                                                                        "e6f5: 3305260\n"
                                                                                        "g7h6: 3275892\n"
                                                                                        "g7f8: 2829099\n"
                                                                                        "a8b8: 2886295\n"
                                                                                        "a8c8: 2849219\n"
                                                                                        "a8d8: 2592939\n"
                                                                                        "h8f8: 2840069\n"
                                                                                        "h8g8: 3006344\n"
                                                                                        "d7e7: 3394851\n"
                                                                                        "d7c8: 3069792\n"
                                                                                        "d7d8: 3243081\n"
                                                                                        "e8e7: 3818306\n"
                                                                                        "e8d8: 2855379\n"
                                                                                        "e8f8: 2999602\n"
                                                                                        "e8g8: 3120184\n"
                                                                                        "e8c8: 2527354");
                                                                                                             */
    //return 0;

    fen_perft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, "a2a3: 106743106\n"
                                                                             "b2b3: 133233975\n"
                                                                             "c2c3: 144074944\n"
                                                                             "d2d3: 227598692\n"
                                                                             "e2e3: 306138410\n"
                                                                             "f2f3: 102021008\n"
                                                                             "g2g3: 135987651\n"
                                                                             "h2h3: 106678423\n"
                                                                             "a2a4: 137077337\n"
                                                                             "b2b4: 134087476\n"
                                                                             "c2c4: 157756443\n"
                                                                             "d2d4: 269605599\n"
                                                                             "e2e4: 309478263\n"
                                                                             "f2f4: 119614841\n"
                                                                             "g2g4: 130293018\n"
                                                                             "h2h4: 138495290\n"
                                                                             "b1a3: 120142144\n"
                                                                             "b1c3: 148527161\n"
                                                                             "g1f3: 147678554\n"
                                                                             "g1h3: 120669525");

    fen_perft("r3k2r/pppq1pbp/3pbnp1/3Pp3/1P1nP3/2NQBNP1/P1P2PBP/R2K3R b kq - 1 1",3,
              "g6g5: 1779\n"
              "a7a6: 1823\n"
              "b7b6: 1773\n"
              "c7c6: 1745\n"
              "h7h6: 1823\n"
              "a7a5: 1947\n"
              "b7b5: 1544\n"
              "c7c5: 1949\n"
              "h7h5: 1862\n"
              "d4c2: 1950\n"
              "d4e2: 1776\n"
              "d4b3: 1908\n"
              "d4f3: 1614\n"
              "d4b5: 1613\n"
              "d4f5: 1861\n"
              "d4c6: 1773\n"
              "f6e4: 2065\n"
              "f6g4: 1819\n"
              "f6d5: 2023\n"
              "f6h5: 1817\n"
              "f6g8: 1730\n"
              "e6h3: 1840\n"
              "e6g4: 1536\n"
              "e6d5: 2106\n"
              "e6f5: 1860\n"
              "g7h6: 1856\n"
              "g7f8: 1732\n"
              "a8b8: 1775\n"
              "a8c8: 1733\n"
              "a8d8: 1690\n"
              "h8f8: 1732\n"
              "h8g8: 1775\n"
              "d7a4: 2057\n"
              "d7b5: 1981\n"
              "d7c6: 2032\n"
              "d7e7: 1786\n"
              "d7c8: 1662\n"
              "d7d8: 1743\n"
              "e8e7: 2074\n"
              "e8d8: 1772\n"
              "e8f8: 1778\n"
              "e8g8: 1821\n"
              "e8c8: 1690");

    fen_perft("r4k1r/pppq1pbp/3pbnp1/3Pp3/1P1nP3/2NQBNP1/P1P2PBP/R2K3R w KQkq - 1 1",
              2,
              "a2a3: 44\n"
              "h2h3: 44\n"
              "g3g4: 43\n"
              "b4b5: 42\n"
              "a2a4: 44\n"
              "h2h4: 44\n"
              "d5e6: 44\n"
              "c3b1: 44\n"
              "c3e2: 44\n"
              "c3a4: 44\n"
              "c3b5: 42\n"
              "f3e1: 44\n"
              "f3g1: 44\n"
              "f3d2: 44\n"
              "f3d4: 38\n"
              "f3h4: 44\n"
              "f3e5: 45\n"
              "f3g5: 43\n"
              "g2f1: 44\n"
              "g2h3: 44\n"
              "e3c1: 44\n"
              "e3d2: 44\n"
              "e3d4: 38\n"
              "e3f4: 45\n"
              "e3g5: 43\n"
              "e3h6: 42\n"
              "a1b1: 44\n"
              "a1c1: 44\n"
              "h1e1: 44\n"
              "h1f1: 44\n"
              "h1g1: 44\n"
              "d3f1: 44\n"
              "d3d2: 44\n"
              "d3e2: 44\n"
              "d3c4: 44\n"
              "d3d4: 38\n"
              "d3b5: 42\n"
              "d3a6: 43\n"
              "d1c1: 44\n"
              "d1e1: 44\n"
              "d1d2: 44\n"
              "d1c1: 44\n"
              "d1g1: 44");



    fen_perft("rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 0 1",
              6,
              "a7a6: 5524004\n"
              "b7b6: 6571198\n"
              "c7c6: 6727202\n"
              "d7d6: 10024535\n"
              "e7e6: 12158064\n"
              "f7f6: 5442844\n"
              "g7g6: 6615575\n"
              "h7h6: 5546006\n"
              "a7a5: 6646627\n"
              "b7b5: 6558608\n"
              "c7c5: 7303220\n"
              "d7d5: 10945177\n"
              "e7e5: 12202299\n"
              "f7f5: 6051164\n"
              "g7g5: 6481170\n"
              "h7h5: 6645540\n"
              "b8a6: 6016445\n"
              "b8c6: 7051465\n"
              "g8f6: 7113069\n"
              "g8h6: 6054342");




    fen_perft("rnbqkbnr/ppppp1pp/8/5p2/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 2", 5,
              "a2a3: 215256\n"
              "b2b3: 252440\n"
              "c2c3: 259637\n"
              "d2d3: 389934\n"
              "e2e3: 365052\n"
              "g2g3: 257217\n"
              "h2h3: 245752\n"
              "a2a4: 254097\n"
              "b2b4: 253795\n"
              "c2c4: 277612\n"
              "d2d4: 400326\n"
              "e2e4: 415692\n"
              "g2g4: 291864\n"
              "h2h4: 250090\n"
              "b1a3: 234442\n"
              "b1c3: 274278\n"
              "f3g1: 197641\n"
              "f3d4: 271802\n"
              "f3h4: 218239\n"
              "f3e5: 268496\n"
              "f3g5: 239815\n"
              "h1g1: 217687");



    fen_perft("rnbqkbnr/ppppp1pp/8/5p2/8/4PN2/PPPP1PPP/RNBQKB1R b KQkq - 0 2",4,
              "f5f4: 17597\n"
              "a7a6: 15785\n"
              "b7b6: 17285\n"
              "c7c6: 17339\n"
              "d7d6: 19344\n"
              "e7e6: 24640\n"
              "g7g6: 17346\n"
              "h7h6: 15697\n"
              "a7a5: 17379\n"
              "b7b5: 16334\n"
              "c7c5: 18075\n"
              "d7d5: 20128\n"
              "e7e5: 24666\n"
              "g7g5: 17345\n"
              "h7h5: 17291\n"
              "b8a6: 16457\n"
              "b8c6: 18160\n"
              "g8f6: 19798\n"
              "g8h6: 16474\n"
              "e8f7: 17912");

    fen_perft("rnbqkbnr/ppppp1pp/8/8/5p2/4PN2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",3,
              "a2a3: 559\n"
              "b2b3: 599\n"
              "c2c3: 599\n"
              "d2d3: 579\n"
              "g2g3: 648\n"
              "h2h3: 599\n"
              "e3e4: 531\n"
              "a2a4: 599\n"
              "b2b4: 600\n"
              "c2c4: 561\n"
              "d2d4: 659\n"
              "g2g4: 629\n"
              "h2h4: 599\n"
              "e3f4: 533\n"
              "b1a3: 579\n"
              "b1c3: 639\n"
              "f3g1: 626\n"
              "f3d4: 710\n"
              "f3h4: 626\n"
              "f3e5: 680\n"
              "f3g5: 642\n"
              "f1e2: 599\n"
              "f1d3: 678\n"
              "f1c4: 678\n"
              "f1b5: 593\n"
              "f1a6: 613\n"
              "h1g1: 559\n"
              "d1e2: 560\n"
              "e1e2: 521");

    fen_perft("rnbqkbnr/ppppp1pp/8/8/5pP1/4PN2/PPPP1P1P/RNBQKB1R b KQkq g3 0 3",2,
              "a7a6: 30\n"
              "b7b6: 30\n"
              "c7c6: 30\n"
              "d7d6: 30\n"
              "e7e6: 30\n"
              "g7g6: 30\n"
              "h7h6: 30\n"
              "a7a5: 30\n"
              "b7b5: 29\n"
              "c7c5: 30\n"
              "d7d5: 30\n"
              "e7e5: 30\n"
              "g7g5: 29\n"
              "h7h5: 31\n"
              "f4e3: 30\n"
              "f4g3: 30\n"
              "b8a6: 30\n"
              "b8c6: 30\n"
              "g8f6: 30\n"
              "g8h6: 30\n"
              "e8f7: 30");
    BoardGUI gui2;
    chess::board board;
    board.set_default_position();
    board.from_fen("rnbqkbnr/ppppp1pp/8/5p2/8/4PN2/PPPP1PPP/RNBQKB1R b KQkq - 0 2");
    //board.make_move(chess::move("g2g4"));
    //board.make_move(chess::move("a7a5"));
    //board.make_move(chess::move("c6d4"));

    cout << board.to_fen() << endl;
    gui2.Play(board);


    return 0;
}