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


#include "timer.h"
#include <unordered_map>

std::unordered_map<std::string, std::chrono::time_point<std::chrono::system_clock>> timers;

std::unordered_map<std::string, unsigned> running_totals;

void create_running_total(std::string name)
{
    running_totals[name] = 0;
}

void add_to_running_total(std::string total_name, std::string timer_name)
{
    running_totals[total_name] += std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::high_resolution_clock::now() - timers[timer_name]).count();
}

unsigned get_running_total(std::string total_name)
{
    return running_totals[total_name];
}

void create_timer(std::string name)
{
    timers[name] = std::chrono::high_resolution_clock::now();
}

unsigned get_elapsed_ms(std::string timer_name)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::high_resolution_clock::now() - timers[timer_name]).count();
}


