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

#include "../common/memory.hpp"
#include "../common/error.hpp"
#include "dram_config.hpp"

namespace dram
{

/**
 * @class DRAM
 * @brief Models a DRAm component
 */
class DRAM: public Memory {
    DRAMConfig& dram_config; ///< dram coniguration
    std::string socket_path;  ///< socket path

    int setup_client();
public:    
    /**
     * @brief DRAM constructor
     */
    DRAM(DRAMConfig& config, std::string socket_path);

    /**
     * @brief access an address
     */
    void access(uint32 address, bool isWrite, std::vector<uint8>& data) override;
};

} // namespace dram
