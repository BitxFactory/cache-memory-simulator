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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "dram_bank.hpp"

namespace dram
{

const uint32 base_cycles = 12;
const uint32 base_rows = 512;
const uint32 base_cols = 1024;
const uint32 scale = 0.3;

/**
 * @struct DRAMRequest
 * @brief  memory request
 */
struct DRAMRequest {
    uint64 seq;       ///< arrival order
    uint32 row_addr;  ///< row address
    uint32 col_addr;  ///< column address

    bool is_write;    ///< transfer mode
    std::vector<uint8>& data; ///< data

    int client_fd;  ///< set by server

    /**
     * @brief DRAMRequest constructor
     */
    DRAMRequest(uint32 r, uint32 c, bool is_write, std::vector<uint8>& d);
};

/**
 * @struct DRAMResponse
 * @brief memory response
 */
struct DRAMResponse {
    uint64 seq;       ///< arrival order
    uint64 completion_cycle; ///< cycles to complete
    bool success;    ///< response success
    std::vector<uint8> data; ///< data for read requests
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
     * @brief setup controller server
     */
    void setup_server();

    /**
     * @brief run the memory controller
     */
    void run();

private:
    void handle_request(int c_fd);

    void dispatcher_loop();

    DRAMTiming calculate_timings();

    std::vector<std::unique_ptr<DRAMBankEqClass>> eq_class;
    DRAMConfig& dram_config;

    DRAMTiming timings;

    std::string socket_path;
    int controller_fd = -1;

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

    bool dispatcher_stop = false;
}; 

} // namespace dram
