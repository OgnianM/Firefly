/*
 This file is part of Leela Chess Zero.
 Copyright (C) 2018-2019 The LCZero Authors
 Leela Chess is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 Leela Chess is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with Leela Chess.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LC0_EXTRACTED_UTILS
#define LC0_EXTRACTED_UTILS

#include <cstdint>
#include <string>
#include <zlib.h>
#include <stdexcept>

enum BoardTransform {
  NoTransform = 0,
  // Horizontal mirror, ReverseBitsInBytes
  FlipTransform = 1,
  // Vertical mirror, ReverseBytesInBytes
  MirrorTransform = 2,
  // Diagonal transpose A1 to H8, TransposeBitsInBytes.
  TransposeTransform = 4,
};

inline uint64_t ReverseBitsInBytes(uint64_t v) {
    v = ((v >> 1) & 0x5555555555555555ull) | ((v & 0x5555555555555555ull) << 1);
    v = ((v >> 2) & 0x3333333333333333ull) | ((v & 0x3333333333333333ull) << 2);
    v = ((v >> 4) & 0x0F0F0F0F0F0F0F0Full) | ((v & 0x0F0F0F0F0F0F0F0Full) << 4);
    return v;
}

inline uint64_t ReverseBytesInBytes(uint64_t v) {
    v = (v & 0x00000000FFFFFFFF) << 32 | (v & 0xFFFFFFFF00000000) >> 32;
    v = (v & 0x0000FFFF0000FFFF) << 16 | (v & 0xFFFF0000FFFF0000) >> 16;
    v = (v & 0x00FF00FF00FF00FF) << 8 | (v & 0xFF00FF00FF00FF00) >> 8;
    return v;
}

// Transpose across the diagonal connecting bit 7 to bit 56.
inline uint64_t TransposeBitsInBytes(uint64_t v) {
    v = (v & 0xAA00AA00AA00AA00ULL) >> 9 | (v & 0x0055005500550055ULL) << 9 |
        (v & 0x55AA55AA55AA55AAULL);
    v = (v & 0xCCCC0000CCCC0000ULL) >> 18 | (v & 0x0000333300003333ULL) << 18 |
        (v & 0x3333CCCC3333CCCCULL);
    v = (v & 0xF0F0F0F000000000ULL) >> 36 | (v & 0x000000000F0F0F0FULL) << 36 |
        (v & 0x0F0F0F0FF0F0F0F0ULL);
    return v;
}

typedef std::invalid_argument Exception;

/*
inline std::string DecompressGzip(const std::string& filename) {
    const int kStartingSize = 8 * 1024 * 1024;  // 8M
    std::string buffer;
    buffer.resize(kStartingSize);
    int bytes_read = 0;

    // Read whole file into a buffer.
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        throw Exception("Cannot read weights from " + filename);
    }
    if (filename == CommandLine::BinaryName()) {
        // The network file should be appended at the end of the lc0 executable,
        // followed by the network file size and a "Lc0!" (0x2130634c) magic.
        int32_t size, magic;
        if (fseek(fp, -8, SEEK_END) || fread(&size, 4, 1, fp) != 1 ||
            fread(&magic, 4, 1, fp) != 1 || magic != 0x2130634c) {
            fclose(fp);
            throw Exception("No embedded file detected.");
        }
        fseek(fp, -size - 8, SEEK_END);
    }
    fflush(fp);
    gzFile file = gzdopen(dup(fileno(fp)), "rb");
    fclose(fp);
    if (!file) {
        throw Exception("Cannot process file " + filename);
    }
    while (true) {
        const int sz =
                gzread(file, &buffer[bytes_read], buffer.size() - bytes_read);
        if (sz < 0) {
            int errnum;
            throw Exception(gzerror(file, &errnum));
        }
        if (sz == static_cast<int>(buffer.size()) - bytes_read) {
            bytes_read = buffer.size();
            buffer.resize(buffer.size() * 2);
        } else {
            bytes_read += sz;
            buffer.resize(bytes_read);
            break;
        }
    }
    gzclose(file);

    return buffer;
}

*/

#endif