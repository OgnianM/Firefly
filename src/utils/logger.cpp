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

#include "logger.h"
#include <iostream>

using namespace logging;
std::unordered_map<std::string, log_writer> all_loggers;

log_writer& logging::log(std::string name)
{
    return all_loggers[name];
}

log_writer& logging::init_log_writer(std::string name, std::string file_path)
{
    all_loggers[name].init(file_path);
    return all_loggers[name];
}


void logging::flush_all()
{
    for (auto& i : all_loggers)
    {
        if (i.second.initialized)
        i.second.ofs.flush();
    }
}