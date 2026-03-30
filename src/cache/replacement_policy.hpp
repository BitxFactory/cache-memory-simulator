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

#include <memory>
#include <string_view>
#include <vector>
#include <optional>

#include "../common/utility.hpp"

/**
 * @enum ReplacementPolicyType
 * @brief replacement policies
 */
enum class ReplacementPolicyType {
    FIFO
};

/**
 * @class ReplacementPolicy
 * @brief Abstract Base class for replacement policies
 */
class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;

    /**
     * @brief update states on cache access
     * @param blockInd index of accessed block
     */
    virtual void onAccess(uint32 blockInd) = 0;

    /**
     * @brief update policy state on block insert
     * @param blockInd index of installed block
     */
    virtual void onInsert(uint32 blockInd) = 0;

    /**
     * @brief Select victim block for replacement
     * @param validBlocks Bit vector indicating which blocks are valid
     * @return index of victim block
     */
    virtual uint32 getVictim(const std::vector<bool>& validBlocks) = 0;

    /**
     * @brief reset policy state
     */
    virtual void reset() = 0;

    /**
     * @brief get policy name
     * @return policy name
     */
    virtual std::string_view getName() const = 0;
};

/**
 * @class FIFOPolicy
 * @brief first-in first-out replacement policy
 */
class FIFOPolicy: public ReplacementPolicy {
public:   
    explicit FIFOPolicy(int numBlocks)
        : order(numBlocks)
    {
        reset();
    }

    void onAccess(uint32 blockInd) override {
        // no change in order
        (void)blockInd;
    }

    void onInsert(uint32 blockInd) override {
        order[blockInd] = ++counter;
    }

    uint32 getVictim(const std::vector<bool>& validBlocks) override {
        std::optional<uint32> victim;
        uint32 maxCounter = UINT32_MAX;

        for (uint32 i = 0; i < validBlocks.size(); i++) {
            if (validBlocks[i] && order[i] < maxCounter) {
                maxCounter = order[i];
                victim = i;
            }
        }

        return (victim.has_value()) ? *victim : 0;
    }

    void reset() override {
        std::fill(order.begin(), order.end(), 0);
        counter = 0;
    }

    std::string_view getName() const override {
        return "FIFO";
    }

private:
    // FIFO policy states
    std::vector<uint32> order;
    uint32 counter;
};

std::unique_ptr<ReplacementPolicy> CreateReplacementPolicy(ReplacementPolicyType policy, uint32 numBlocks) {

}