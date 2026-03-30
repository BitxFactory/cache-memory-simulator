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

#include "replacement_policy.hpp"

namespace cache
{

// FIFO Policy implementations
void FIFOPolicy::onAccess(uint32 blockInd) {
    // no state change on access
    (void)blockInd;
}

void FIFOPolicy::onInsert(uint32 blockInd) {
    order[blockInd] = ++counter;
}

uint32 FIFOPolicy::getVictim(const std::vector<bool>& validBlocks)  {
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

std::string_view FIFOPolicy::getName() const {
    return "FIFO";
}

void FIFOPolicy::reset() {
    std::fill(order.begin(), order.end(), 0);
    counter = 0;
}

} // namespace cache