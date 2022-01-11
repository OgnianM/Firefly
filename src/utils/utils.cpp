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


#include "utils.h"
#include <random>
#include <algorithm>


void print_bitboard(uint64_t bb)
{
    uint64_t bit = 1;
    for (int x = 0; x < 8; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            if (bb & bit)
                cout << '1';
            else cout << '0';

            bit <<= 1;
            cout << "  ";
        }
        cout << '\n';
    }
    cout << endl;
}


void print_bitboard_list(std::vector<uint64_t> list, std::vector<std::string> labels)
{
    for (int i = 0; i < labels.size(); i++)
    {
        cout << labels[i];
        for (int y = labels[i].size(); y < 9*3; y++)
            cout << ' ';
    }
    cout << endl;
    for (int y = 0; y < 8; y++)
    {
        for (int i = 0; i < list.size(); i++)
        {
            for (int x = 0; x < 8; x++)
            {
                if (list[i] & get_bit(x,y))
                    cout << "1  ";
                else cout << "0  ";
            }
            cout << "   ";
        }
        cout << '\n';
    }
    cout << endl;
}








uint64_t random64()
{
    static std::uniform_int_distribution<uint64_t> dist;
    return dist(get_rng());
}
float random_float()
{
    static std::uniform_real_distribution<float> dist;
    return dist(get_rng());
}
double random_double()
{
    static std::uniform_real_distribution<double> dist;
    return dist(get_rng());
}
float random_normalized()
{
    static std::uniform_real_distribution<float> dist(0, 1);
    return dist(get_rng());
}

