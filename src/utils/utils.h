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

#ifndef FIREFLY_UTILS_H
#define FIREFLY_UTILS_H


#include <iostream>
#include "bit_manip.h"
#include <vector>
#include <string>
#include <random>
#include "logger.h"

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;

void print_bitboard(uint64_t bb);
void print_bitboard_list(std::vector<uint64_t> list, std::vector<std::string> labels);

inline std::mt19937_64& get_rng()
{
    static std::random_device entropy_source;
    thread_local std::mt19937_64 generator(entropy_source());
    return generator;
}


uint64_t random64();
float random_float();
double random_double();

float random_normalized();



int probabilistic_select(std::vector<float> const& probabilities);
int maximum_select(std::vector<float> const& values);
int minimum_select(std::vector<float> const& values);
#endif //FIREFLY_UTILS_H
