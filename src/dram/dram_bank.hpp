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

#include <dram_array.hpp>

namespace dram
{

/**
 * @struct DRAMBank
 * @brief DRAM Bank
 */
struct DRAMBank {

    /**
     * @brief DRAMBank construtor
     */
    DRAMBank(uint32 id, DRAMConfig& config);

    /**
     * @brief precharge subarray and activate corresponding rows
     */
    void open_row(uint32 row_addr);

    /**
     * @brief transfer data from dram to cpu
     */
    void transfer_data(uint32 col_addr, bool is_write, std::span<uint8> data);

private:
    uint32 id; ///< bank id
    std::vector<std::unique_ptr<DRAMSubarray>> arrays; ///< subarrays of the bank

    DRAMConfig& dram_config;
};

/**
 * @struct DRAMEqClass
 * @brief DRAM bank equivalence class
 */
struct DRAMBankEqClass {
    /**
     * @brief DRAMBankEqClass constructor
     */
    DRAMBankEqClass(uint32 id, DRAMConfig& config);

    /**
     * @brief precharge subarray and activate corresponding rows of all banks in an eq class
     */
    void open_row(uint32 row_addr);

    /**
     * @brief transfer data from dram to cpu
     */
    void transfer_data(uint32 col_addr, bool is_write, std::span<uint8> data);

private:    
    uint32 id;  ///< class id
    std::vector<std::unique_ptr<DRAMBank>> banks;  ///< banks in a eq class
    DRAMConfig& dram_config;
};

} // namespace dram
