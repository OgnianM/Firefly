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

#ifndef FIREFLY_BOARD_TEST_H
#define FIREFLY_BOARD_TEST_H

#include "../chess/board.h"

void board_stress_test();
void test_pos(string fen);
void test_pos(chess::board b);
void fen_perft(string fen, int depth, string expected_values="");

#endif //FIREFLY_BOARD_TEST_H
