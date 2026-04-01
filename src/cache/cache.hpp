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
#include <memory>
#include <optional>
#include <span>

#include "cache_block.hpp"
#include "../common/memory.hpp"
#include "replacement_policy.hpp"

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
 * @enum CacheMissType
 * @brief type of cache misses
 */
enum class CacheMissType {
    CompulsoryMiss,   ///< compulsory miss
    ConflictMiss,     ///< conflict miss
    CapacityMiss      ///< capacity miss
};

/**
 * @enum WritePolicyType
 * @brief Type of write policies
 */
enum class WritePolicyType {
    WriteThrough,    ///< write-through policy
    WriteBack        ///< write-back policy
};

/**
 * @struct CacheConfig
 * @brief cache configuration
 */
struct CacheConfig {
    uint32 size;           ///< size of cache in bytes
    uint32 associativity;  ///< number of ways in a set
    uint32 blockSize;      ///< size of each cache block in bytes
    ReplacementPolicyType policy; ///< replacement policy
    WritePolicyType writePolicy;  ///< write policy

    uint32 accessLatency;         ///< latency in cycles to access the cache
    uint32 updateLatency;         ///< latency to update the cache
    uint32 upstreamLinkLatency;   ///< latency to transfer data to upper level (cycles/block)
    uint32 downstreamLinkLatency; ///< latency to transfer data to lower level (cycles/block)

    CacheConfig() = default;

    CacheConfig(uint32 sz, uint32 assoc, uint32 blockSize,
                ReplacementPolicyType policy = ReplacementPolicyType::FIFO,
                WritePolicyType writePolicy = WritePolicyType::WriteBack,
                uint32 accessLatency = 4, uint32 updateLatency = 4,
                uint32 upstreamLinkLatency = 8, uint32 downstreamLinkLatency = 8)
        : size(sz), associativity(assoc),
          blockSize(blockSize), policy(policy), writePolicy(writePolicy),
          accessLatency(accessLatency), updateLatency(updateLatency),
          upstreamLinkLatency(upstreamLinkLatency), downstreamLinkLatency(downstreamLinkLatency)
    {
        // basic sanity
        assert(size > 0 && 
                    "size of cache must be a positive number");
        assert(associativity > 0 && 
                    "cache associativity should be atleast 1");
        assert(blockSize > 0 && 
                    "block size of the cache must be a positive integer");
        assert(accessLatency > 0 &&
                    "access latency must be a positive number");
        assert(updateLatency > 0 &&
                    "update latency must be a positive number");
        assert(upstreamLinkLatency > 0 &&
                    "upstream latency must be a positive number");
        assert(downstreamLinkLatency > 0 &&
                    "downstream latency must be a positive number");
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
    explicit Cache(CacheConfig& config);

    void access(uint32 address, bool isWrite, std::vector<uint8>& data) override;

private:
    // cache parameters
    uint32 size;           ///< size of cache in bytes
    uint32 associativity;  ///< associativity of the cache, number of blocks in a set
    uint32 blockSize;      ///< block size in bytes
    uint32 numOfBlocks;    ///< number of blocks (cache lines) in the cache
    uint32 numOfSets;      ///< number of sets cache is logically divided into

    // config
    std::unique_ptr<CacheConfig> config;

    std::optional<std::shared_ptr<Memory>> nextMemoryLevel; ///< next memory level in the memory hierarchy

    // types
    CacheAssociativityType associativityType; ///< type of cache associativity
    CacheMissType          missType;          ///< type of cache miss
    
    // cache tag array and data
    std::vector<CacheSet> sets; ///< cache set
    std::vector<std::unique_ptr<ReplacementPolicy>> replacementPolicySet; ///< replacement policy set

    // Stats
    int hits;   ///< number of hits
    int misses; ///< number of misses
    int reads;  ///< total reads
    int writes; ///< total writes

private:
    // get the tag and set id from the address
    // <-------------address------------->
    // +-------------+----------+--------+
    // |  tag bits   |  set id  | offset |
    // +-------------+----------+--------+

    /**
     * @param address cpu generated address
     * @brief resolves the cpu address to corresponding tag and setId
     * <-------------address------------->
     * +-------------+----------+--------+
     * |  tag bits   |  set id  | offset |
     * +-------------+----------+--------+
     * @returns {tag, setID}
     */
    std::pair<uint32, int> resolveAddress(uint32 address) const;

    /**
     * @param tag the tag bits of the address
     * @param setId the set ID of the address
     * @param [out] blockNum the block number at cache hit
     * @brief check if the block present in the cache
     * @returns true if present, otherwise false
     */
    bool findBlock(uint32 tag, int setId, uint32* blockNum) const;

    /**
     * @param setId the set ID of the address
     * @brief find the available block, if not present replace one
     * @returns the block number
     */
    uint32 createEmptyBlock(int setId) const;

    /**
     * @param address the address
     * @param setId the set ID of the address
     * @param blockInd the block index evicted
     * @brief reads the block from the lower levels and install it
     */
    void installBlock(uint32 address, int setId, uint32 blockInd) const;

    /**
     * @param dst the destination
     * @param src the source
     * @brief copy data from src to dst
     */
    void copyData(std::span<uint8> dst, const std::span<uint8> src);

    /**
     * @param address the address
     * @param data the data to be read/write
     * @brief read/write data to/from bus based on write flag
     */
    void handleDataReadWrite(uint32 address, std::vector<uint8>& data);
};

} // namespace cache