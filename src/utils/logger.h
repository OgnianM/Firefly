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

#ifndef FIREFLY_LOGGER_H
#define FIREFLY_LOGGER_H
#include <string>
#include <fstream>
#include <unordered_map>

namespace logging {
    struct log_writer {
        std::ofstream ofs;
        bool initialized = false;

        inline log_writer& operator<<(auto data)
        {
            if (initialized)
                ofs << data;
            return *this;
        }

        inline log_writer& operator<<(uint8_t byte)
        {
            if (initialized)
                ofs << int(byte);
            return *this;
        }

        inline void init(std::string file)
        {
            ofs.open(file, std::ios::trunc);
            initialized = ofs.good();
        }
    };

    log_writer& log(std::string name = "default");

    log_writer& init_log_writer(std::string name, std::string file_path);

    void flush_all();
};
#endif //FIREFLY_LOGGER_H
