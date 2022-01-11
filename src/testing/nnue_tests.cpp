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

#include <chess/board.h>
#include <external/nnue-probe/src/nnue.h>
#include <utils/utils.h>
#include <testing/timer.h>
void nnue_tests()
{
    nnue_init("nn-62ef826d1a6d.nnue");

    chess::board board;

    board.from_fen("4k3/8/4K3/8/8/3q4/8/8 b - - 0 1");

    cout << board.nnue_eval() << endl;

    /*board.set_default_position();

    create_timer("t1");

    for (int i = 0; i < 100000; i++)
    {
        board.nnue_eval();
    }

    cout << get_elapsed_ms("t1") << endl;
*/
}


int mai9n()
{
    nnue_tests();

    return 0;
}