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

#ifndef BITBOARD_GENERATOR_BIT_MANIP_H
#define BITBOARD_GENERATOR_BIT_MANIP_H

#include <chess/constants/constants.h>
#include <cstdint>
#include <config.h>

typedef uint8_t bit_index;


inline uint8_t get_index(int8_t x, int8_t y)
{
    return x + (y<<3);
}

inline uint64_t get_bit(bit_index idx)
{
    return uint64_t(1) << idx;
}

inline uint64_t get_bit(int8_t x, int8_t y)
{
    return get_bit(get_index(x,y));
    //return (bitmask(1) << x) << (y << 3);
}


inline void flip_bit(uint64_t& m, int8_t x, int8_t y)
{
    m ^= get_bit(x,y);
}

inline void flip_bit(uint64_t& m,  bit_index idx)
{
    m ^= get_bit(idx);
}

inline void flip_bit(uint64_t& m, square_bit square)
{
    m ^= square;
}

inline void set_bit(uint64_t& m, square_bit square)
{
    m |= square;
}

inline void set_bit(uint64_t& m, int8_t x, int8_t y)
{
    m |= get_bit(x,y);
}

inline void set_bit(uint64_t& m, uint8_t index)
{
    m |= get_bit(index);
}

inline void clear_bit(uint64_t& m, uint8_t index)
{
    m &= ~(get_bit(index));
}

inline void clear_bit(uint64_t& m, int8_t x, int8_t y)
{
    clear_bit(m, get_index(x,y));
}

inline void clear_bit(uint64_t& m, square_bit square)
{
    m &= ~square;
}

inline uint64_t reverse_bytes(uint64_t x)
{
    x = (x >> 32) | (x << 32);
    x = ((x & 0xffff0000ffff0000) >> 16) | ((x & 0x0000ffff0000ffff) << 16);
    x = ((x & 0xff00ff00ff00ff00) >> 8)  | ((x & 0x00ff00ff00ff00ff) << 8);
    return x;
}

inline uint64_t poplsb(uint64_t& x)
{
    auto lsb = x&~(x-1);
    x ^= lsb;
    return lsb;
}

inline uint64_t set_difference(uint64_t const& a, uint64_t const& b)
{
    // return a ^ b;
    // return a - b;
    return a &~ b;
}


#endif //BITBOARD_GENERATOR_BIT_MANIP_H





