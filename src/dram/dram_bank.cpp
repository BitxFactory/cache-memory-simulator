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

#include "dram_bank.hpp"
#include "error.hpp"

#include <string>

namespace dram
{

DRAMBank::DRAMBank(uint32 id, DRAMConfig& config): id(id), dram_config(config)
{
    arrays.resize(dram_config.number_of_banks());

    uint32 number_of_rows = 1LL << dram_config.row_bits;
    uint32 number_of_cols = 1LL << dram_config.col_bits;

    for (uint32 i = 0; i < arrays.size(); i++) {
        arrays[i] = std::make_unique<DRAMSubarray>(number_of_rows, number_of_cols);
    }
}

void DRAMBank::open_row(uint32 row_addr)
{
    for (uint32 i = 0; i < arrays.size(); i++) {
        // precharge
        arrays[i]->precharge();

        // activate a row
        if (!arrays[i]->activate(row_addr)) {
            throw DRAMError("Failed to activate DRAM subarray " + std::to_string(i + 1) + " at row " + std::to_string(row_addr));
        }
    }
}

void DRAMBank::transfer_data(uint32 col_addr, bool is_write, std::vector<bool>& data)
{
    // read/write (t)
    for (uint32 i = 0; i < arrays.size(); i++) {
        if (is_write) {
            if (!arrays[i]->write(col_addr, data[i])) {
                throw DRAMError("Failed to write to DRAM subarray " + std::to_string(i + 1) + " at column " + std::to_string(col_addr));
            }
        } else {
            bool temp;
            if (!arrays[i]->read(col_addr, temp)) {
                throw DRAMError("Failed to read from DRAM subarray " + std::to_string(i + 1) + " at column " + std::to_string(col_addr));
            }
            data[i] = temp;
        }
    }
}

BankRequest::BankRequest(uint32 r, uint32 c, bool is_write, std::vector<bool>& data)
    : row_addr(r), col_addr(c), is_write(is_write), data(data) {}

BankWorker::BankWorker(std::unique_ptr<DRAMBank> b, std::unique_ptr<DRAMTiming> t)
    : bank(std::move(b)), timings(std::move(t)), stop(false) {}

void BankWorker::handle_requests()
{
    // event loop
    while (!stop) {
        BankRequest* req = nullptr;

        {
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait(lk, [&]{ return stop || !queue.empty(); });
            req = queue.front();
            queue.pop();
        }

        // open row phase
        uint64 open_row_start = current_cycle;
        bank->open_row(req->row_addr);

        uint64 open_row_end = open_row_start + timings->precharge + timings->activate;
        // signal caller: open row done, next bank can be called
        req->open_row_done.set_value(open_row_end);

        // transfer phase — starts immediately after open_row
        uint64 transfer_end = open_row_end + timings->transfer;
        bank->transfer_data(
            req->col_addr, req->is_write, req->data);

        current_cycle = transfer_end;

        // Signal caller: full request done, here is the completion cycle
        req->result.set_value(transfer_end);

    }
}

} // namespace dram
