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

#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "dram_array.hpp"

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
    void transfer_data(uint32 col_addr, bool is_write, std::vector<bool>& data);

private:
    uint32 id; ///< bank id
    std::vector<std::unique_ptr<DRAMSubarray>> arrays; ///< subarrays of the bank

    DRAMConfig& dram_config;
};

/**
 * @struct DRAMTiming
 * @brief timing of different dram cycle
 */
struct DRAMTiming {
    uint32 precharge;  ///< tRP : time taken to complete a precharge operation in a bank
    uint32 activate;   ///< tRCD: time between an active command and a column command
    uint32 transfer;   ///< tCAS: time to transfer data in/out memory
};

/**
 * @struct BankRequest
 * @brief memory request to a bank
 */
struct BankRequest {
    uint32 row_addr;       ///< row address
    uint32 col_addr;   ///< column address
    bool   is_write;   ///< read or write mode
    std::vector<bool>& data; ///< data to be written, NULL if read mode

    // two promises for
    std::promise<uint64> open_row_done; ///< open row operation (precharge + activate) done
    std::promise<uint64> result; ///< transfer operation done

    /**
     * @brief BankRequest constructor
     */
    BankRequest(uint32 row_addr, uint32 col_addr, bool is_write, std::vector<bool>& data);
};

/**
 * @struct BankWorker
 * @brief worker thread for a bank
 */
struct BankWorker {
    std::unique_ptr<DRAMBank> bank;
    std::unique_ptr<DRAMTiming> timings;

    std::queue<BankRequest*> queue;
    std::mutex               mtx;
    std::condition_variable  cv;
    std::thread              thread;
    bool                     stop;
    uint64                   current_cycle = 0;  // when this bank is next free

    /**
     * @brief BankWorker constructor
     */
    BankWorker(std::unique_ptr<DRAMBank> b, std::unique_ptr<DRAMTiming> t);

    /**
     * @brief handle requests in the queue
     */
    void handle_requests();
};

} // namespace dram
