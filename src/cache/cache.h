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

#include <cassert>
#include <optional>
#include <memory>

#include "cache_block.h"
#include "memory.h"

namespace cache {

/**
 * @enum CacheAssociativityType
 * @brief Type of associativity of cache
 */
enum class CacheAssociativityType {
    DirectMapping,     ///< direct mapped
    SetAssociative,    ///< set-associative
    FullyAssociative   ///< full-associative
};

/**
 * @struct CacheConfig
 * @brief cache configuration
 */
struct CacheConfig {
    uint32 size;           ///< size of cache in bytes
    uint32 associativity;  ///< number of ways in a set
    uint32 blockSize;      ///< size of each cache block in bytes

    CacheConfig() = default;

    CacheConfig(uint32 sz, uint32 assoc, uint32 blockSize): 
        size(sz),
        associativity(assoc),
        blockSize(blockSize)
    {
        // basic sanity
        assert(size > 0 && 
                    "size of cache must be a positive number");
        assert(associativity > 0 && 
                    "cache associativity should be atleast 1");
        assert(blockSize > 0 && 
                    "block size of the cache must be a positive integer");

        // power of 2
        assert((blockSize & (blockSize - 1)) == 0 && 
                    "block size must be power of 2");
        assert((size % blockSize == 0) && 
                    "cache size must be multiple of block size");
        
        uint32 numOfBlocks = number_of_blocks(size, blockSize);

        // clamp associativity
        if (associativity > numOfBlocks) {
            associativity = numOfBlocks;    // case of full associativity
        }

        // check block size divisibility
        assert(numOfBlocks % associativity == 0 && 
                    "number of blocks must be divisible by associativity");
        // cache sets validate
        uint32 numSets = number_of_sets(numOfBlocks, associativity);
        assert((numSets & (numSets - 1)) == 0 && 
                    "number of sets must be a power of 2");
    }
};

/**
 * @struct Cache
 * @brief Models a cache component
 */
class Cache: public Memory {
public:
    explicit Cache(const CacheConfig& config);

    void access(uint32 address, bool isWrite) override;

private:
    // cache parameters
    uint32 size;           ///< size of cache in bytes
    uint32 associativity;  ///< associativity of the cache, number of blocks in a set
    uint32 blockSize;      ///< block size in bytes
    uint32 numOfBlocks;    ///< number of blocks (cache lines) in the cache
    uint32 numOfSets;      ///< number of sets cache is logically divided into

    std::optional<std::shared_ptr<Memory>> nextMemoryLevel; ///< next memory level in the memory hierarchy

    // types
    CacheAssociativityType associativityType; ///< type of cache associativity
    
    // cache tag array and data
    std::vector<CacheSet> sets; ///< cache set

    // Stats
    int hits;   ///< number of hits
    int misses; ///< number of misses
    int reads;  ///< total reads
    int writes; ///< total writes
};

} // namespace cache