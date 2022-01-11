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

#pragma once
#include <atomic>

//#define USE_PRAGMA_PACK_1

#if ATOMIC_BOOL_LOCK_FREE == 2
#define ATOMIC_NODES
#endif

#define SYNCHRONOUS_INFERENCE

//#define DEBUG_CHECKS

typedef float floatx;

#define TRANSPOSITION_TABLES_ENABLED
//#define PROCESS_ALL_EDGES_AT_ONCE

#define MULTITHREADED
