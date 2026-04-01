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

#include <vector>

#include "../common/utility.hpp"

namespace cache {

/**
 * @struct CacheBlock
 * @brief smallest unit of data that can be transferred between main memory and the cache
 */
struct CacheBlock {
    bool valid = false;    ///< valid bit
    bool dirty = false;    ///< dirty bit
    uint32 tag;            ///< tag bits for address loop

    std::vector<uint8> data; ///< Data stored in this block (assuming 1 word = 4bytes)

    uint32 accessCount;      ///< number of times accessed
    uint64 lastAccess;       ///< timestamp of last accessed

    /* CacheBlock Default constructor */
    CacheBlock() {}

    /* CacheBlock constructor */
    CacheBlock(uint32 blockSize, bool valid = false, bool dirty = false, uint32 tag = 0):
        valid(valid),
        dirty(dirty),
        tag(tag)
    {}
};

/**
 * @struct CacheSet
 * @brief cache set structure
 */
struct CacheSet {
    std::vector<CacheBlock> blocks;  ///< cache block constituting the set
};

} // namespace cache