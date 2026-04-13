// MIT License

// Copyright (c) 2026 BitxFactory

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <cassert>
#include <string>

#include <utility.hpp>

constexpr bool is_power_of_2(uint32 v);
constexpr uint8_t log2_of(uint32 v);

struct SystemConfig {
    uint8 address_bus_width;  ///< address bits supported in bits (max 64)
    uint8 data_bus_width;     ///< data bits supported in bits (max 64)

    uint32 block_size;        ///< size of cache line (in bytes)
    uint8  word_size;         ///< CPU word size in bytes (1,2,4,8)

    /**
     * @brief SystemConfig constructor
     */
    SystemConfig(uint8 address, uint8 data, uint8 word_size, uint32 block_size)
    : address_bus_width(address), data_bus_width(data), 
        word_size(word_size), block_size(block_size)
    {}

    /**
     * @brief total addressable memory in bytes
     */
    constexpr uint64 address_space_bytes() const {
        return 1ULL << address_bus_width;
    }

    /**
     * @brief number of cache lines that fit in address space
     */
    constexpr uint64 total_blocks() const {
        return address_space_bytes() / block_size;
    }

    /**
     * @brief how many bus transfers to fill one cache line
     */
    constexpr uint32 transfers_per_block() const {
        return block_size / (data_bus_width / 8);
    }

    /**
     * @brief words per cache line
     */
    constexpr uint32 words_per_block() const {
        return block_size / word_size;
    }

    /**
     * @brief offset bits needed to index a byte within a block
     */
    constexpr uint8_t offset_bits() const {
        return log2_of(block_size);
    }

    /**
     * @brief remaining bits after offset → available for set index + tag
     */
    constexpr uint8_t tag_index_bits() const {
        return address_bus_width - offset_bits();
    }

    // validations
    bool validate(std::string* err = nullptr) const {
        auto fail = [&](const char* msg) -> bool {
            if (err) *err = msg;
            return false;
        };

        if (address_bus_width == 0 || address_bus_width > 64)
            return fail("addr_bits must be in [1, 64]");

        if (!is_power_of_2(block_size))
            return fail("block_size must be a power of 2");

        if ((block_size * 8) < data_bus_width)
            return fail("block_size must be >= data_bus_bytes");

        if ((block_size * 8) % data_bus_width != 0)
            return fail("block_size must be a multiple of data_bus_bytes");

        if (block_size % word_size != 0)
            return fail("block_size must be a multiple of word_size");

        if (!is_power_of_2(data_bus_width))
            return fail("data_bus_bytes must be a power of 2");

        if (data_bus_width > 64)
            return fail("data_bus_bytes cannot exceed 8 (64-bit bus)");

        if (word_size != 1 && word_size != 2 &&
            word_size != 4 && word_size != 8)
            return fail("word_size must be 1, 2, 4, or 8");

        if ((word_size * 8) > data_bus_width)
            return fail("word_size cannot exceed data_bus_bytes");
            
        return true;
    }
};

constexpr bool is_power_of_2(uint32 v) {
        return v > 0 && (v & (v - 1)) == 0;
    }

constexpr uint8_t log2_of(uint32 v) {
    uint8 n = 0;
    while (v >>= 1) ++n;
    return n;
}