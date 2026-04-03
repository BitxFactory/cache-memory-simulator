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

#include "dram_array.hpp"

namespace dram
{

DRAMSubarray::DRAMSubarray(uint32 rows, uint32 cols)
: rows(rows), cols(cols), is_word_open(false)
{
    cells.resize(rows, std::vector<bool>(cols));
}

void DRAMSubarray::precharge() {
    if (!is_word_open) return;
    // write sense amplifier back to cells
    cells[open_row] = row_buffer;
    is_word_open = false;
}

bool DRAMSubarray::activate(uint32 row_addr) {
    if (is_word_open) return false;
    if (row_addr >= rows) return false;

    row_buffer = cells[row_addr];
    open_row = row_addr;
    return true;
}

bool DRAMSubarray::read(uint32 col_addr, bool& out) {
    if (open_row < 0) return false;
    if (col_addr >= cols) return false;

    out = row_buffer[col_addr];
    return true;
}

bool DRAMSubarray::write(uint32 col_addr, bool data) {
    if (open_row < 0) return false;
    if (col_addr >= cols) return false;

    row_buffer[col_addr] = data;
    return true;
}

} // namespace dram
