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

typedef std::uint8_t uint8;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

#define BYTE 1        // in bytes
#define HALF_WORD 2   // in bytes
#define WORD 4        // in bytes
#define DOUBLE_WORD 8 // in bytes

uint32 number_of_blocks(uint32 size, uint32 blockSize) {
    return size / blockSize;
}

uint32 number_of_sets(uint32 numBlocks, uint32 associativity) {
    return numBlocks / associativity;
}

uint32 line_address(uint32 address, uint32 blockSize) {
    return address / blockSize;
}

uint32 offset(uint32 address, uint32 blockSize) {
    return address % blockSize;
}