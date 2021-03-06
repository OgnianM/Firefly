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

#include "constants.h"

uint64_t knight_attacks[64] = {
        0x0000000000020400ULL,0x0000000000050800ULL,0x00000000000a1100ULL,0x0000000000142200ULL,0x0000000000284400ULL,
        0x0000000000508800ULL,0x0000000000a01000ULL,0x0000000000402000ULL,0x0000000002040004ULL,0x0000000005080008ULL,
        0x000000000a110011ULL,0x0000000014220022ULL,0x0000000028440044ULL,0x0000000050880088ULL,0x00000000a0100010ULL,
        0x0000000040200020ULL,0x0000000204000402ULL,0x0000000508000805ULL,0x0000000a1100110aULL,0x0000001422002214ULL,
        0x0000002844004428ULL,0x0000005088008850ULL,0x000000a0100010a0ULL,0x0000004020002040ULL,0x0000020400040200ULL,
        0x0000050800080500ULL,0x00000a1100110a00ULL,0x0000142200221400ULL,0x0000284400442800ULL,0x0000508800885000ULL,
        0x0000a0100010a000ULL,0x0000402000204000ULL,0x0002040004020000ULL,0x0005080008050000ULL,0x000a1100110a0000ULL,
        0x0014220022140000ULL,0x0028440044280000ULL,0x0050880088500000ULL,0x00a0100010a00000ULL,0x0040200020400000ULL,
        0x0204000402000000ULL,0x0508000805000000ULL,0x0a1100110a000000ULL,0x1422002214000000ULL,0x2844004428000000ULL,
        0x5088008850000000ULL,0xa0100010a0000000ULL,0x4020002040000000ULL,0x0400040200000000ULL,0x0800080500000000ULL,
        0x1100110a00000000ULL,0x2200221400000000ULL,0x4400442800000000ULL,0x8800885000000000ULL,0x100010a000000000ULL,
        0x2000204000000000ULL,0x0004020000000000ULL,0x0008050000000000ULL,0x00110a0000000000ULL,0x0022140000000000ULL,
        0x0044280000000000ULL,0x0088500000000000ULL,0x0010a00000000000ULL,0x0020400000000000ULL,
};

uint64_t white_pawn_attacks[64] = {
        0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,
        0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000020000ULL,0x0000000000050000ULL,
        0x00000000000a0000ULL,0x0000000000140000ULL,0x0000000000280000ULL,0x0000000000500000ULL,0x0000000000a00000ULL,
        0x0000000000400000ULL,0x0000000002000000ULL,0x0000000005000000ULL,0x000000000a000000ULL,0x0000000014000000ULL,
        0x0000000028000000ULL,0x0000000050000000ULL,0x00000000a0000000ULL,0x0000000040000000ULL,0x0000000200000000ULL,
        0x0000000500000000ULL,0x0000000a00000000ULL,0x0000001400000000ULL,0x0000002800000000ULL,0x0000005000000000ULL,
        0x000000a000000000ULL,0x0000004000000000ULL,0x0000020000000000ULL,0x0000050000000000ULL,0x00000a0000000000ULL,
        0x0000140000000000ULL,0x0000280000000000ULL,0x0000500000000000ULL,0x0000a00000000000ULL,0x0000400000000000ULL,
        0x0002000000000000ULL,0x0005000000000000ULL,0x000a000000000000ULL,0x0014000000000000ULL,0x0028000000000000ULL,
        0x0050000000000000ULL,0x00a0000000000000ULL,0x0040000000000000ULL,0x0200000000000000ULL,0x0500000000000000ULL,
        0x0a00000000000000ULL,0x1400000000000000ULL,0x2800000000000000ULL,0x5000000000000000ULL,0xa000000000000000ULL,
        0x4000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,
        0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,
};
uint64_t black_pawn_attacks[64] = {
        0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,
        0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000002ULL,0x0000000000000005ULL,
        0x000000000000000aULL,0x0000000000000014ULL,0x0000000000000028ULL,0x0000000000000050ULL,0x00000000000000a0ULL,
        0x0000000000000040ULL,0x0000000000000200ULL,0x0000000000000500ULL,0x0000000000000a00ULL,0x0000000000001400ULL,
        0x0000000000002800ULL,0x0000000000005000ULL,0x000000000000a000ULL,0x0000000000004000ULL,0x0000000000020000ULL,
        0x0000000000050000ULL,0x00000000000a0000ULL,0x0000000000140000ULL,0x0000000000280000ULL,0x0000000000500000ULL,
        0x0000000000a00000ULL,0x0000000000400000ULL,0x0000000002000000ULL,0x0000000005000000ULL,0x000000000a000000ULL,
        0x0000000014000000ULL,0x0000000028000000ULL,0x0000000050000000ULL,0x00000000a0000000ULL,0x0000000040000000ULL,
        0x0000000200000000ULL,0x0000000500000000ULL,0x0000000a00000000ULL,0x0000001400000000ULL,0x0000002800000000ULL,
        0x0000005000000000ULL,0x000000a000000000ULL,0x0000004000000000ULL,0x0000020000000000ULL,0x0000050000000000ULL,
        0x00000a0000000000ULL,0x0000140000000000ULL,0x0000280000000000ULL,0x0000500000000000ULL,0x0000a00000000000ULL,
        0x0000400000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,
        0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,0x0000000000000000ULL,
};

uint64_t king_moves[64] = {
        0x0000000000000302ULL,0x0000000000000705ULL,0x0000000000000e0aULL,0x0000000000001c14ULL,0x0000000000003828ULL,
        0x0000000000007050ULL,0x000000000000e0a0ULL,0x000000000000c040ULL,0x0000000000030203ULL,0x0000000000070507ULL,
        0x00000000000e0a0eULL,0x00000000001c141cULL,0x0000000000382838ULL,0x0000000000705070ULL,0x0000000000e0a0e0ULL,
        0x0000000000c040c0ULL,0x0000000003020300ULL,0x0000000007050700ULL,0x000000000e0a0e00ULL,0x000000001c141c00ULL,
        0x0000000038283800ULL,0x0000000070507000ULL,0x00000000e0a0e000ULL,0x00000000c040c000ULL,0x0000000302030000ULL,
        0x0000000705070000ULL,0x0000000e0a0e0000ULL,0x0000001c141c0000ULL,0x0000003828380000ULL,0x0000007050700000ULL,
        0x000000e0a0e00000ULL,0x000000c040c00000ULL,0x0000030203000000ULL,0x0000070507000000ULL,0x00000e0a0e000000ULL,
        0x00001c141c000000ULL,0x0000382838000000ULL,0x0000705070000000ULL,0x0000e0a0e0000000ULL,0x0000c040c0000000ULL,
        0x0003020300000000ULL,0x0007050700000000ULL,0x000e0a0e00000000ULL,0x001c141c00000000ULL,0x0038283800000000ULL,
        0x0070507000000000ULL,0x00e0a0e000000000ULL,0x00c040c000000000ULL,0x0302030000000000ULL,0x0705070000000000ULL,
        0x0e0a0e0000000000ULL,0x1c141c0000000000ULL,0x3828380000000000ULL,0x7050700000000000ULL,0xe0a0e00000000000ULL,
        0xc040c00000000000ULL,0x0203000000000000ULL,0x0507000000000000ULL,0x0a0e000000000000ULL,0x141c000000000000ULL,
        0x2838000000000000ULL,0x5070000000000000ULL,0xa0e0000000000000ULL,0x40c0000000000000ULL,
};

uint64_t* pawn_attacks[2] = {white_pawn_attacks, black_pawn_attacks};
uint64_t pawn_promotion_squares = 0xFF000000000000FFULL;


// 64 * LEFT, RIGHT, DOWN, UP
uint64_t rook_pin_rays[64][4] =
{
    {  0x0000000000000000ULL, 0x00000000000000feULL, 0x0101010101010100ULL, 0x0000000000000000ULL, },
    {  0x0000000000000001ULL, 0x00000000000000fcULL, 0x0202020202020200ULL, 0x0000000000000000ULL, },
    {  0x0000000000000003ULL, 0x00000000000000f8ULL, 0x0404040404040400ULL, 0x0000000000000000ULL, },
    {  0x0000000000000007ULL, 0x00000000000000f0ULL, 0x0808080808080800ULL, 0x0000000000000000ULL, },
    {  0x000000000000000fULL, 0x00000000000000e0ULL, 0x1010101010101000ULL, 0x0000000000000000ULL, },
    {  0x000000000000001fULL, 0x00000000000000c0ULL, 0x2020202020202000ULL, 0x0000000000000000ULL, },
    {  0x000000000000003fULL, 0x0000000000000080ULL, 0x4040404040404000ULL, 0x0000000000000000ULL, },
    {  0x000000000000007fULL, 0x0000000000000000ULL, 0x8080808080808000ULL, 0x0000000000000000ULL, },
    {  0x0000000000000000ULL, 0x000000000000fe00ULL, 0x0101010101010000ULL, 0x0000000000000001ULL, },
    {  0x0000000000000100ULL, 0x000000000000fc00ULL, 0x0202020202020000ULL, 0x0000000000000002ULL, },
    {  0x0000000000000300ULL, 0x000000000000f800ULL, 0x0404040404040000ULL, 0x0000000000000004ULL, },
    {  0x0000000000000700ULL, 0x000000000000f000ULL, 0x0808080808080000ULL, 0x0000000000000008ULL, },
    {  0x0000000000000f00ULL, 0x000000000000e000ULL, 0x1010101010100000ULL, 0x0000000000000010ULL, },
    {  0x0000000000001f00ULL, 0x000000000000c000ULL, 0x2020202020200000ULL, 0x0000000000000020ULL, },
    {  0x0000000000003f00ULL, 0x0000000000008000ULL, 0x4040404040400000ULL, 0x0000000000000040ULL, },
    {  0x0000000000007f00ULL, 0x0000000000000000ULL, 0x8080808080800000ULL, 0x0000000000000080ULL, },
    {  0x0000000000000000ULL, 0x0000000000fe0000ULL, 0x0101010101000000ULL, 0x0000000000000101ULL, },
    {  0x0000000000010000ULL, 0x0000000000fc0000ULL, 0x0202020202000000ULL, 0x0000000000000202ULL, },
    {  0x0000000000030000ULL, 0x0000000000f80000ULL, 0x0404040404000000ULL, 0x0000000000000404ULL, },
    {  0x0000000000070000ULL, 0x0000000000f00000ULL, 0x0808080808000000ULL, 0x0000000000000808ULL, },
    {  0x00000000000f0000ULL, 0x0000000000e00000ULL, 0x1010101010000000ULL, 0x0000000000001010ULL, },
    {  0x00000000001f0000ULL, 0x0000000000c00000ULL, 0x2020202020000000ULL, 0x0000000000002020ULL, },
    {  0x00000000003f0000ULL, 0x0000000000800000ULL, 0x4040404040000000ULL, 0x0000000000004040ULL, },
    {  0x00000000007f0000ULL, 0x0000000000000000ULL, 0x8080808080000000ULL, 0x0000000000008080ULL, },
    {  0x0000000000000000ULL, 0x00000000fe000000ULL, 0x0101010100000000ULL, 0x0000000000010101ULL, },
    {  0x0000000001000000ULL, 0x00000000fc000000ULL, 0x0202020200000000ULL, 0x0000000000020202ULL, },
    {  0x0000000003000000ULL, 0x00000000f8000000ULL, 0x0404040400000000ULL, 0x0000000000040404ULL, },
    {  0x0000000007000000ULL, 0x00000000f0000000ULL, 0x0808080800000000ULL, 0x0000000000080808ULL, },
    {  0x000000000f000000ULL, 0x00000000e0000000ULL, 0x1010101000000000ULL, 0x0000000000101010ULL, },
    {  0x000000001f000000ULL, 0x00000000c0000000ULL, 0x2020202000000000ULL, 0x0000000000202020ULL, },
    {  0x000000003f000000ULL, 0x0000000080000000ULL, 0x4040404000000000ULL, 0x0000000000404040ULL, },
    {  0x000000007f000000ULL, 0x0000000000000000ULL, 0x8080808000000000ULL, 0x0000000000808080ULL, },
    {  0x0000000000000000ULL, 0x000000fe00000000ULL, 0x0101010000000000ULL, 0x0000000001010101ULL, },
    {  0x0000000100000000ULL, 0x000000fc00000000ULL, 0x0202020000000000ULL, 0x0000000002020202ULL, },
    {  0x0000000300000000ULL, 0x000000f800000000ULL, 0x0404040000000000ULL, 0x0000000004040404ULL, },
    {  0x0000000700000000ULL, 0x000000f000000000ULL, 0x0808080000000000ULL, 0x0000000008080808ULL, },
    {  0x0000000f00000000ULL, 0x000000e000000000ULL, 0x1010100000000000ULL, 0x0000000010101010ULL, },
    {  0x0000001f00000000ULL, 0x000000c000000000ULL, 0x2020200000000000ULL, 0x0000000020202020ULL, },
    {  0x0000003f00000000ULL, 0x0000008000000000ULL, 0x4040400000000000ULL, 0x0000000040404040ULL, },
    {  0x0000007f00000000ULL, 0x0000000000000000ULL, 0x8080800000000000ULL, 0x0000000080808080ULL, },
    {  0x0000000000000000ULL, 0x0000fe0000000000ULL, 0x0101000000000000ULL, 0x0000000101010101ULL, },
    {  0x0000010000000000ULL, 0x0000fc0000000000ULL, 0x0202000000000000ULL, 0x0000000202020202ULL, },
    {  0x0000030000000000ULL, 0x0000f80000000000ULL, 0x0404000000000000ULL, 0x0000000404040404ULL, },
    {  0x0000070000000000ULL, 0x0000f00000000000ULL, 0x0808000000000000ULL, 0x0000000808080808ULL, },
    {  0x00000f0000000000ULL, 0x0000e00000000000ULL, 0x1010000000000000ULL, 0x0000001010101010ULL, },
    {  0x00001f0000000000ULL, 0x0000c00000000000ULL, 0x2020000000000000ULL, 0x0000002020202020ULL, },
    {  0x00003f0000000000ULL, 0x0000800000000000ULL, 0x4040000000000000ULL, 0x0000004040404040ULL, },
    {  0x00007f0000000000ULL, 0x0000000000000000ULL, 0x8080000000000000ULL, 0x0000008080808080ULL, },
    {  0x0000000000000000ULL, 0x00fe000000000000ULL, 0x0100000000000000ULL, 0x0000010101010101ULL, },
    {  0x0001000000000000ULL, 0x00fc000000000000ULL, 0x0200000000000000ULL, 0x0000020202020202ULL, },
    {  0x0003000000000000ULL, 0x00f8000000000000ULL, 0x0400000000000000ULL, 0x0000040404040404ULL, },
    {  0x0007000000000000ULL, 0x00f0000000000000ULL, 0x0800000000000000ULL, 0x0000080808080808ULL, },
    {  0x000f000000000000ULL, 0x00e0000000000000ULL, 0x1000000000000000ULL, 0x0000101010101010ULL, },
    {  0x001f000000000000ULL, 0x00c0000000000000ULL, 0x2000000000000000ULL, 0x0000202020202020ULL, },
    {  0x003f000000000000ULL, 0x0080000000000000ULL, 0x4000000000000000ULL, 0x0000404040404040ULL, },
    {  0x007f000000000000ULL, 0x0000000000000000ULL, 0x8000000000000000ULL, 0x0000808080808080ULL, },
    {  0x0000000000000000ULL, 0xfe00000000000000ULL, 0x0000000000000000ULL, 0x0001010101010101ULL, },
    {  0x0100000000000000ULL, 0xfc00000000000000ULL, 0x0000000000000000ULL, 0x0002020202020202ULL, },
    {  0x0300000000000000ULL, 0xf800000000000000ULL, 0x0000000000000000ULL, 0x0004040404040404ULL, },
    {  0x0700000000000000ULL, 0xf000000000000000ULL, 0x0000000000000000ULL, 0x0008080808080808ULL, },
    {  0x0f00000000000000ULL, 0xe000000000000000ULL, 0x0000000000000000ULL, 0x0010101010101010ULL, },
    {  0x1f00000000000000ULL, 0xc000000000000000ULL, 0x0000000000000000ULL, 0x0020202020202020ULL, },
    {  0x3f00000000000000ULL, 0x8000000000000000ULL, 0x0000000000000000ULL, 0x0040404040404040ULL, },
    {  0x7f00000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0080808080808080ULL, },
};

uint64_t bishop_pin_rays[64][4]
{
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x8040201008040200ULL, 0x0000000000000000ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0080402010080400ULL, 0x0000000000000100ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000804020100800ULL, 0x0000000000010200ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000008040201000ULL, 0x0000000001020400ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000080402000ULL, 0x0000000102040800ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000804000ULL, 0x0000010204081000ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000008000ULL, 0x0001020408102000ULL, },
{  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0102040810204000ULL, },
{  0x0000000000000000ULL, 0x0000000000000002ULL, 0x4020100804020000ULL, 0x0000000000000000ULL, },
{  0x0000000000000001ULL, 0x0000000000000004ULL, 0x8040201008040000ULL, 0x0000000000010000ULL, },
{  0x0000000000000002ULL, 0x0000000000000008ULL, 0x0080402010080000ULL, 0x0000000001020000ULL, },
{  0x0000000000000004ULL, 0x0000000000000010ULL, 0x0000804020100000ULL, 0x0000000102040000ULL, },
{  0x0000000000000008ULL, 0x0000000000000020ULL, 0x0000008040200000ULL, 0x0000010204080000ULL, },
{  0x0000000000000010ULL, 0x0000000000000040ULL, 0x0000000080400000ULL, 0x0001020408100000ULL, },
{  0x0000000000000020ULL, 0x0000000000000080ULL, 0x0000000000800000ULL, 0x0102040810200000ULL, },
{  0x0000000000000040ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0204081020400000ULL, },
{  0x0000000000000000ULL, 0x0000000000000204ULL, 0x2010080402000000ULL, 0x0000000000000000ULL, },
{  0x0000000000000100ULL, 0x0000000000000408ULL, 0x4020100804000000ULL, 0x0000000001000000ULL, },
{  0x0000000000000201ULL, 0x0000000000000810ULL, 0x8040201008000000ULL, 0x0000000102000000ULL, },
{  0x0000000000000402ULL, 0x0000000000001020ULL, 0x0080402010000000ULL, 0x0000010204000000ULL, },
{  0x0000000000000804ULL, 0x0000000000002040ULL, 0x0000804020000000ULL, 0x0001020408000000ULL, },
{  0x0000000000001008ULL, 0x0000000000004080ULL, 0x0000008040000000ULL, 0x0102040810000000ULL, },
{  0x0000000000002010ULL, 0x0000000000008000ULL, 0x0000000080000000ULL, 0x0204081020000000ULL, },
{  0x0000000000004020ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0408102040000000ULL, },
{  0x0000000000000000ULL, 0x0000000000020408ULL, 0x1008040200000000ULL, 0x0000000000000000ULL, },
{  0x0000000000010000ULL, 0x0000000000040810ULL, 0x2010080400000000ULL, 0x0000000100000000ULL, },
{  0x0000000000020100ULL, 0x0000000000081020ULL, 0x4020100800000000ULL, 0x0000010200000000ULL, },
{  0x0000000000040201ULL, 0x0000000000102040ULL, 0x8040201000000000ULL, 0x0001020400000000ULL, },
{  0x0000000000080402ULL, 0x0000000000204080ULL, 0x0080402000000000ULL, 0x0102040800000000ULL, },
{  0x0000000000100804ULL, 0x0000000000408000ULL, 0x0000804000000000ULL, 0x0204081000000000ULL, },
{  0x0000000000201008ULL, 0x0000000000800000ULL, 0x0000008000000000ULL, 0x0408102000000000ULL, },
{  0x0000000000402010ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0810204000000000ULL, },
{  0x0000000000000000ULL, 0x0000000002040810ULL, 0x0804020000000000ULL, 0x0000000000000000ULL, },
{  0x0000000001000000ULL, 0x0000000004081020ULL, 0x1008040000000000ULL, 0x0000010000000000ULL, },
{  0x0000000002010000ULL, 0x0000000008102040ULL, 0x2010080000000000ULL, 0x0001020000000000ULL, },
{  0x0000000004020100ULL, 0x0000000010204080ULL, 0x4020100000000000ULL, 0x0102040000000000ULL, },
{  0x0000000008040201ULL, 0x0000000020408000ULL, 0x8040200000000000ULL, 0x0204080000000000ULL, },
{  0x0000000010080402ULL, 0x0000000040800000ULL, 0x0080400000000000ULL, 0x0408100000000000ULL, },
{  0x0000000020100804ULL, 0x0000000080000000ULL, 0x0000800000000000ULL, 0x0810200000000000ULL, },
{  0x0000000040201008ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x1020400000000000ULL, },
{  0x0000000000000000ULL, 0x0000000204081020ULL, 0x0402000000000000ULL, 0x0000000000000000ULL, },
{  0x0000000100000000ULL, 0x0000000408102040ULL, 0x0804000000000000ULL, 0x0001000000000000ULL, },
{  0x0000000201000000ULL, 0x0000000810204080ULL, 0x1008000000000000ULL, 0x0102000000000000ULL, },
{  0x0000000402010000ULL, 0x0000001020408000ULL, 0x2010000000000000ULL, 0x0204000000000000ULL, },
{  0x0000000804020100ULL, 0x0000002040800000ULL, 0x4020000000000000ULL, 0x0408000000000000ULL, },
{  0x0000001008040201ULL, 0x0000004080000000ULL, 0x8040000000000000ULL, 0x0810000000000000ULL, },
{  0x0000002010080402ULL, 0x0000008000000000ULL, 0x0080000000000000ULL, 0x1020000000000000ULL, },
{  0x0000004020100804ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x2040000000000000ULL, },
{  0x0000000000000000ULL, 0x0000020408102040ULL, 0x0200000000000000ULL, 0x0000000000000000ULL, },
{  0x0000010000000000ULL, 0x0000040810204080ULL, 0x0400000000000000ULL, 0x0100000000000000ULL, },
{  0x0000020100000000ULL, 0x0000081020408000ULL, 0x0800000000000000ULL, 0x0200000000000000ULL, },
{  0x0000040201000000ULL, 0x0000102040800000ULL, 0x1000000000000000ULL, 0x0400000000000000ULL, },
{  0x0000080402010000ULL, 0x0000204080000000ULL, 0x2000000000000000ULL, 0x0800000000000000ULL, },
{  0x0000100804020100ULL, 0x0000408000000000ULL, 0x4000000000000000ULL, 0x1000000000000000ULL, },
{  0x0000201008040201ULL, 0x0000800000000000ULL, 0x8000000000000000ULL, 0x2000000000000000ULL, },
{  0x0000402010080402ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x4000000000000000ULL, },
{  0x0000000000000000ULL, 0x0002040810204080ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0001000000000000ULL, 0x0004081020408000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0002010000000000ULL, 0x0008102040800000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0004020100000000ULL, 0x0010204080000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0008040201000000ULL, 0x0020408000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0010080402010000ULL, 0x0040800000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0020100804020100ULL, 0x0080000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
{  0x0040201008040201ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, },
};


uint64_t white_king_side_castles_mask = 96;
uint64_t white_queen_side_castles_mask = 0x000000000000000cULL;
uint64_t black_king_side_castles_mask = 0x6000000000000000ULL;
uint64_t black_queen_side_castles_mask = 0x0c00000000000000ULL;
