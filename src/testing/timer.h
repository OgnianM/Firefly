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

#ifndef FIREFLY_TIMER_H
#define FIREFLY_TIMER_H

#include <string>
#include <chrono>

void create_running_total(std::string name);
void add_to_running_total(std::string total_name, std::string timer_name);
unsigned get_running_total(std::string total_name);
void create_timer(std::string name);
unsigned get_elapsed_ms(std::string timer_name);

inline uint64_t cycle_timer()
{
    return __builtin_ia32_rdtsc();
}

#endif //FIREFLY_TIMER_H
