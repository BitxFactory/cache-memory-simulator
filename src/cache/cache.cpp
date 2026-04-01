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

#include <atomic>

#include "cache.hpp"

namespace cache
{

static std::atomic<uint64> globalCounter = {0};

Cache::Cache(CacheConfig& conf)
    : size(conf.size), associativity(conf.associativity), blockSize(conf.blockSize)
{
    numOfBlocks = number_of_blocks(size, blockSize);
    numOfSets   = number_of_sets(numOfBlocks, associativity);

    // calculate associativity type
    if (associativity == 1) {
        associativityType = CacheAssociativityType::DirectMapping;
    } else if (associativity == numOfBlocks) {
        associativityType = CacheAssociativityType::FullyAssociative;
    } else {
        associativityType = CacheAssociativityType::SetAssociative;
    }

    config = std::make_unique<CacheConfig>(conf);

    // Initialize cache
    sets.resize(numOfSets);
    replacementPolicySet.resize(numOfSets);

    for (uint32 i = 0; i < numOfSets; i++) {
        auto &set = sets[i];
        set.blocks.resize(associativity);

        for (uint32 j = 0; j < associativity; j++) {
            auto &block = set.blocks[j];
            block.data.resize(blockSize);
        }

        replacementPolicySet[i] = ReplacementPolicyFactory(config->policy, associativity);
    }
}

void Cache::access(uint32 address, bool isWrite, std::vector<uint8>& data) {
    // resolve the address  
    auto [tag, setId] = resolveAddress(address);
    auto &set = sets[setId];

    // try finding block in cache
    uint32 blockId;
    CacheBlock block;

    if (findBlock(tag, setId, &blockId)) {  // hit
        hits++;

        block = set.blocks[blockId];

        block.accessCount++;
        block.lastAccess = globalCounter.fetch_add(1);

        replacementPolicySet[setId]->onAccess(blockId);
    } else {                                // miss
        misses++;

        // find victim block
        blockId = createEmptyBlock(setId);
        auto &victimBlock = set.blocks[blockId];

        // write back the evicted block if dirty
        if (victimBlock.valid && victimBlock.dirty) {
            uint32 victimAddress = (victimBlock.tag * numOfSets + setId) * blockSize;
            if (nextMemoryLevel.has_value())
                nextMemoryLevel.value()->access(victimAddress, true, data);
        }

        // insert new block
        if (nextMemoryLevel.has_value()) {
            nextMemoryLevel.value()->access(address, false, data);
        }

        block = set.blocks[blockId];
        block.valid = true;
        block.tag = tag;

        block.accessCount = 1;
        block.lastAccess = globalCounter.fetch_add(1);

        // update replacement states for inserted block
        replacementPolicySet[setId]->onInsert(blockId);  
    }

    if (isWrite) {          // write
        writes++;
        if (config->writePolicy == WritePolicyType::WriteThrough) {
            block.dirty = false;
            if (nextMemoryLevel.has_value())
                nextMemoryLevel.value()->access(address, true, data);
        } else {
            block.dirty = true;
        }

        // write data in the block
        copyData(block.data, data);
    } else {                // read
        reads++;
        copyData(data, block.data);
    }
}

std::pair<uint32, int> Cache::resolveAddress(uint32 address) const {
    uint32 lineAddress = line_address(address, blockSize);
    int setInd = lineAddress % numOfSets;
    uint32 tag = lineAddress / numOfSets;
    return {tag, setInd};
}

bool Cache::findBlock(uint32 tag, int setId, uint32* blockNum) const {
    auto &set = sets[setId];
    for (uint32 i = 0; i < set.blocks.size(); i++) {
        const auto &block = set.blocks[i];
        if (block.valid && block.tag == tag) {  // hit
            *blockNum = i;
            return true;
        }
    }

    return false;
}

uint32 Cache::createEmptyBlock(int setId) const {
    auto &set = sets[setId];

    // find invalid block before eviction
    for (uint32 i = 0; i < set.blocks.size(); i++) {
        if (!set.blocks[i].valid) {
            return i;
        }
    }

    // all ways are filled, evict one
    std::vector<bool> validBlocks(associativity, true);
    return replacementPolicySet[setId]->getVictim(validBlocks);
}

void Cache::copyData(std::span<uint8> dst, const std::span<uint8> src) {
    assert(dst.size() == src.size() && "size mismatch");
    std::copy(src.begin(), src.end(), dst.begin());
}

} // namespace cache
