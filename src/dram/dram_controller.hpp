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
#include <mutex>
#include <queue>
#include <thread>

#include "../common/error.hpp"
#include "dram_bank.hpp"

namespace dram
{

const uint32 base_cycles = 12;
const uint32 base_rows = 512;
const uint32 base_cols = 1024;
const uint32 scale = 0.3;

/**
 * @struct DRAMBankResult
 * @brief per-bank output
 */
struct DRAMBankResult {
    uint64 completion_cycle;  ///< time of completion of request
    std::vector<uint8> data;  ///< data read
};

/**
 * @struct DRAMTiming
 * @brief memory controller command cycles
 */
struct DRAMTiming {
    uint32 precharge;   // tRP  cycles
    uint32 activate;    // tRCD cycles
    uint32 transfer;    // tCAS cycles
};

/**
 * @struct DRAMController
 * @brief 
 */
struct DRAMController {
    /**
     * @brief DRAMController constructor
     */
    DRAMController(DRAMConfig& config, std::string socket_path);

    /**
     * @brief DRAMController destructor
     */
    ~DRAMController();

    /**
     * @brief start the memory controller
     */
    void start();

    /**
     * @brief gracefully shutdown memory controller
     */
    void stop();

private:
    void setup_server();

    void handle_request(int c_fd);

    void dispatcher_loop();

    DRAMTiming calculate_timings();

    uint64 get_bank_id(uint64 address);
    uint64 get_row_id(uint64 address);
    uint64 get_col_id(uint64 address);

    std::vector<std::unique_ptr<DRAMBankEqClass>> eq_class; ///< equivalent class of dram banks
    DRAMConfig& dram_config;  ///< dram configuration

    DRAMTiming timings;  ///< timing associated with dram memory access cycle

    int controller_fd = -1;   ///< file descriptor of controller server
    std::string socket_path;  ///< socket path
    std::atomic<bool> server_stop{false}; ///< flag to stop server

    struct SeqCmp {
        bool operator()(const DRAMRequest* a, const DRAMRequest* b) {
            return a->seq > b->seq;
        }
    };

    std::priority_queue<DRAMRequest*, std::vector<DRAMRequest*>, SeqCmp> 
                dispatcher_queue;   ///< dispatcher queue (FCFS based on arrival)
    std::thread dispatcher_thread;  ///< dispatcher thread
    std::mutex  dispatcher_mtx;     ///< dispatcher mutex lock
    std::condition_variable dispatcher_cv; ///< dispatcher conditional variable

    bool dispatcher_stop = false;       ///< flag to stop dispatcher thread
    std::atomic<uint64> current_cycle;  ///< cycles elapsed (accessing memory)
    std::atomic<uint64> next_seq;       ///< counter for client requests
}; 

} // namespace dram
