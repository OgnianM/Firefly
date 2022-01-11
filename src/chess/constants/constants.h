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

#ifndef FIREFLY_CONSTANTS_H
#define FIREFLY_CONSTANTS_H

#include <cstdint>
#include "bishop_attacks.h"
#include "rook_attacks.h"

extern uint64_t white_pawn_attacks[64];
//extern uint64_t white_pawn_pushes[64];
extern uint64_t black_pawn_attacks[64];
//extern uint64_t black_pawn_pushes[64];



//extern uint64_t* pawn_pushes[2];
extern uint64_t* pawn_attacks[2];
extern uint64_t pawn_promotion_squares;

extern uint64_t king_moves[64];
extern uint64_t knight_attacks[64];

extern uint64_t bishop_pin_rays[64][4];
extern uint64_t rook_pin_rays[64][4];

extern uint64_t white_king_side_castles_mask,
 white_queen_side_castles_mask ,
 black_king_side_castles_mask,
 black_queen_side_castles_mask;

static const char* startpos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


#define C_WHITE 0
#define C_BLACK 1

#define WHITE_KING_SIDE 1
#define WHITE_QUEEN_SIDE 2
#define BLACK_KING_SIDE 4
#define BLACK_QUEEN_SIDE 8

enum square_bit : uint64_t
{
    a1 = 1,
    b1 = 2,
    c1 = 4,
    d1 = 8,
    e1 = 16,
    f1 = 32,
    g1 = 64,
    h1 = 128,
    a2 = 256,
    b2 = 512,
    c2 = 1024,
    d2 = 2048,
    e2 = 4096,
    f2 = 8192,
    g2 = 16384,
    h2 = 32768,
    a3 = 65536,
    b3 = 131072,
    c3 = 262144,
    d3 = 524288,
    e3 = 1048576,
    f3 = 2097152,
    g3 = 4194304,
    h3 = 8388608,
    a4 = 16777216,
    b4 = 33554432,
    c4 = 67108864,
    d4 = 134217728,
    e4 = 268435456,
    f4 = 536870912,
    g4 = 1073741824,
    h4 = 2147483648,
    a5 = 4294967296,
    b5 = 8589934592,
    c5 = 17179869184,
    d5 = 34359738368,
    e5 = 68719476736,
    f5 = 137438953472,
    g5 = 274877906944,
    h5 = 549755813888,
    a6 = 1099511627776,
    b6 = 2199023255552,
    c6 = 4398046511104,
    d6 = 8796093022208,
    e6 = 17592186044416,
    f6 = 35184372088832,
    g6 = 70368744177664,
    h6 = 140737488355328,
    a7 = 281474976710656,
    b7 = 562949953421312,
    c7 = 1125899906842624,
    d7 = 2251799813685248,
    e7 = 4503599627370496,
    f7 = 9007199254740992,
    g7 = 18014398509481984,
    h7 = 36028797018963968,
    a8 = 72057594037927936,
    b8 = 144115188075855872,
    c8 = 288230376151711744,
    d8 = 576460752303423488,
    e8 = 1152921504606846976,
    f8 = 2305843009213693952,
    g8 = 4611686018427387904,
    h8 = 9223372036854775808ULL
};

enum class square_index : uint8_t
{
    a1 = 0,
    b1 = 1,
    c1 = 2,
    d1 = 3,
    e1 = 4,
    f1 = 5,
    g1 = 6,
    h1 = 7,
    a2 = 8,
    b2 = 9,
    c2 = 10,
    d2 = 11,
    e2 = 12,
    f2 = 13,
    g2 = 14,
    h2 = 15,
    a3 = 16,
    b3 = 17,
    c3 = 18,
    d3 = 19,
    e3 = 20,
    f3 = 21,
    g3 = 22,
    h3 = 23,
    a4 = 24,
    b4 = 25,
    c4 = 26,
    d4 = 27,
    e4 = 28,
    f4 = 29,
    g4 = 30,
    h4 = 31,
    a5 = 32,
    b5 = 33,
    c5 = 34,
    d5 = 35,
    e5 = 36,
    f5 = 37,
    g5 = 38,
    h5 = 39,
    a6 = 40,
    b6 = 41,
    c6 = 42,
    d6 = 43,
    e6 = 44,
    f6 = 45,
    g6 = 46,
    h6 = 47,
    a7 = 48,
    b7 = 49,
    c7 = 50,
    d7 = 51,
    e7 = 52,
    f7 = 53,
    g7 = 54,
    h7 = 55,
    a8 = 56,
    b8 = 57,
    c8 = 58,
    d8 = 59,
    e8 = 60,
    f8 = 61,
    g8 = 62,
    h8 = 63
};

#endif //FIREFLY_CONSTANTS_H
