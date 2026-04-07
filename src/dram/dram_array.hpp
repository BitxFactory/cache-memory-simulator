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

#include <span>
#include <vector>

#include "dram_config.hpp"

namespace dram
{

/**
 * @struct DRAMSubarray
 * @brief subarray of a bank
 */
struct DRAMSubarray {
    /**
     * @brief constructor of DRAMSubarray
     */
    DRAMSubarray(uint32 rows, uint32 cols);

    /**
     * @brief write back sense amplifier data to cells & close row
     */
    void precharge();

    /**
     * @brief activate a word line
     * @param r word line number
     * @returns true if successful otherwise false
     */
    bool activate(uint32 row_addr);

    /**
     * @brief read a bit
     * @param col column number
     * @returns true if successul otherwise false
     */
    bool read(uint32 col_addr, uint8& out);

    /**
     * @brief write a bit
     * @param col column number
     * @returns true if successul otherwise false
     */
    bool write(uint32 col_addr, uint8 data);

    /**
     * @brief get row view
     * @param r row number
     * @returns a span
     */
    std::span<uint8> get_row(uint32 r);

private:
    uint32 rows;            ///< number of rows in the subarray
    uint32 cols;            ///< number of cols in the subarray

    uint32_t open_row;      ///< opened word line
    bool is_word_open;      ///< is word line active
    std::vector<uint8> row_buffer; ///< data latched in sense amplifier
    std::vector<uint8> storage; ///< the actual data
};

} // namespace dram
