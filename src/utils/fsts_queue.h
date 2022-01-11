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

#ifndef FIREFLY_FSTS_QUEUE_H
#define FIREFLY_FSTS_QUEUE_H


/*
 * Originally for use with an asynchronous network manager.
 * Kinda useless now
 */

#include <cstdint>
#include <atomic>
#include <thread>

/*
 * fixed size lockless thread-safe queue.
 */
template<typename T, size_t SleepIntervalNS=0>
struct fsts_queue
{
    const uint64_t Size;
    const uint64_t M;

    T* data;

    std::atomic<uint64_t> front, back, write_pointer, read_pointer;
    std::atomic<bool> try_write_lock, try_read_lock;

    fsts_queue(size_t Exponent) : front(0), back(0), write_pointer(0), read_pointer(0),
                                  Size(1 << Exponent), M(Size - 1), try_write_lock(false), try_read_lock(false)
    {
        data = new T[Size];
    }

    ~fsts_queue()
    {
        delete[] data;
    }

    bool empty() const noexcept
    {
        return front == back;
    }


    inline void clear() noexcept
    {
        front = 0, back = 0, write_pointer = 0, read_pointer = 0;
    }

    [[nodiscard]] inline size_t size() const noexcept
    {
        return back - front;
    }

    [[nodiscard]] inline size_t capacity() const noexcept
    {
        return Size;
    }


    bool try_push(const T& in) noexcept
    {
        bool f = false;
        while (!try_write_lock.compare_exchange_weak(f, true))
            f = false;

        if (write_pointer >= (front + Size))
        {
            try_write_lock = false;
            return false;
        }

        auto index = write_pointer++;
        try_write_lock = false;

        data[index & M] = in;

        while (back != index);
        back++;
        return true;
    }

    bool try_pop(T& out) noexcept
    {
        bool f = false;
        while (!try_read_lock.compare_exchange_weak(f, true))
            f = false;

        if (read_pointer >= back)
        {
            try_read_lock = false;
            return false;
        }

        auto index = read_pointer++;
        try_read_lock = false;

        out = data[index & M];

        while (front != index);
        front++;

        return true;
    }


    void push(const T& in) noexcept
    {
        auto index = write_pointer++;

        while (index >= (front + Size)) if constexpr(SleepIntervalNS != 0)
                std::this_thread::sleep_for(std::chrono::nanoseconds(SleepIntervalNS));


        data[index & M] = in;         // Set data

        // Block until all threads with reserved indices lower than index have done their work
        // This is necessary because otherwise a thread with index == 3 might finish its work before another
        // thread with index == 1, if that first thread increments back before the second thread, that might prompt
        // a reader to read data from index == 1, that has not been successfully written yet.

        while (back != index);
        back++;                       // Increment back to signal that data is available for reading
    }



    // Blocks if queue is empty

    T pop() noexcept
    {
        auto index = read_pointer++;

        while (index >= back) if constexpr(SleepIntervalNS != 0)
                std::this_thread::sleep_for(std::chrono::nanoseconds(SleepIntervalNS));


        auto out = data[index & M];

        while (front != index);
        front++;

        return out;
    }
};

#endif //FIREFLY_FSTS_QUEUE_H
