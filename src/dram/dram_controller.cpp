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

#include <cstring>
#include <filesystem>
#include <future>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>

#include <dram_controller.hpp>

namespace dram
{

DRAMRequest::DRAMRequest(uint64 address, bool is_write, std::vector<uint8> d)
    : address(address), is_write(is_write), data(d) {}

DRAMController::DRAMController(DRAMConfig& config, std::string socket_path)
    : dram_config(config), socket_path(socket_path)
{
    // initialize timing constraints
    timings = calculate_timings();
    // create banks
    eq_class.resize(dram_config.number_of_banks());
    for (uint32 i = 0; i < dram_config.number_of_banks(); i++) {
        eq_class[i] = std::make_unique<DRAMBankEqClass>(i + 1, config);
    }
}

DRAMController::~DRAMController()
{ 
    stop();
}

void DRAMController::start() {
    setup_server();
    dispatcher_thread = std::thread(&DRAMController::dispatcher_loop, this);

    while (!server_stop)
    {
        int client_fd = ::accept(controller_fd, nullptr, nullptr);
        if (client_fd < 0) break;   // stop() closed server_fd_

        // Each client gets a handler thread
        std::thread([this, client_fd] {
            handle_request(client_fd);
        }).detach();
    }
}

void DRAMController::stop()
{
    server_stop = true;

    // unblock accept
    ::close(controller_fd);

    // unblock dispatcher
    {
        std::lock_guard<std::mutex> lk(dispatcher_mtx);
        dispatcher_stop = true;
    }

    dispatcher_cv.notify_one();
    if (dispatcher_thread.joinable()) dispatcher_thread.join();
}

void DRAMController::setup_server()
{
    controller_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (controller_fd == -1)
        throw std::runtime_error("socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (socket_path.size() >= sizeof(addr.sun_path))
        throw std::runtime_error("Socket path too long");

    std::memcpy(addr.sun_path, socket_path.c_str(), socket_path.size());

    std::filesystem::remove(socket_path);

    if (::bind(controller_fd,
               reinterpret_cast<sockaddr*>(&addr),
               sizeof(addr)) == -1)
        throw std::runtime_error("bind() failed");

    if (::listen(controller_fd, 8) == -1)
        throw std::runtime_error("listen() failed");
}

DRAMTiming DRAMController::calculate_timings()
{
    DRAMTiming timing;

    uint32 num_rows = (1LL << dram_config.row_bits);
    uint32 num_cols = (1LL << dram_config.col_bits);

    timings.precharge = timings.activate = base_cycles * (num_rows / base_rows);
    timings.transfer = base_cycles + (scale * log2_of(num_cols / base_cols));

    return timing;
}

void DRAMController::dispatcher_loop()
{
    uint64 expected_seq = 0;
    std::unordered_map<uint32, std::future<DRAMBankResult>> last_transfer;

    while (!dispatcher_stop) {
        DRAMRequest* req = nullptr;
        {
            std::unique_lock<std::mutex> lk(dispatcher_mtx);
            dispatcher_cv.wait(lk, [&]{
                return dispatcher_stop || 
                    (!dispatcher_queue.empty() && dispatcher_queue.top()->seq == expected_seq);
            });
            if (dispatcher_stop) return;

            req = dispatcher_queue.top();
            dispatcher_queue.pop();
        }
        expected_seq++;

        uint32 bidx = get_bank_id(req->address);

        // same bank wait for previous transfer to fully complete
        if (last_transfer.count(bidx)) {
            auto& f = last_transfer[bidx];
            f.wait();   // blocks if in-flight, instant if already done
            last_transfer.erase(bidx);  // safe to erase now — we just waited on it
        }

        // open row cycle
        uint64 opne_row_start = current_cycle.load();
        eq_class[bidx]->open_row(get_row_id(req->address));
        uint64 open_row_end = opne_row_start + timings.precharge + timings.activate;
        current_cycle.store(open_row_end);

        uint32 col = get_col_id(req->address);
        bool is_write = req->is_write;
        int cfd = req->client_fd;
        uint64 seq = req->seq;
        std::vector<uint8> data;
        
        std::copy(req->data.begin(), req->data.end(), data.begin());
        delete req;

        // transfer cycle, dispatcher can handle next request now
        last_transfer[bidx] = std::async(std::launch::async, [this, bidx, col, 
            is_write, cfd, seq, data, opne_row_start, open_row_end]() mutable -> DRAMBankResult {
                // sleep (for other thread )
                uint64 transfer_cycle_start = open_row_end;
                eq_class[bidx]->transfer_data(col, is_write, data);
                uint64 transfer_cycle_end = transfer_cycle_start + timings.transfer;

                current_cycle.store(std::max(current_cycle.load(), transfer_cycle_end));

                // send result to client and close connection
                DRAMResponse resp{};
                resp.seq = seq;
                resp.completion_cycle = transfer_cycle_end - opne_row_start;
                resp.success = true;
                if (!is_write)
                    for (uint32_t i = 0; i < data.size(); i++)
                        resp.data[i] = data[i];
                
                ssize_t n = ::send(cfd, &resp, sizeof(resp), 0);
                if (n < 0)
                    throw DRAMError("failed to send response for request number: " + std::to_string(resp.seq));

                return DRAMBankResult{ resp.completion_cycle, data };
        });
    }
}

void DRAMController::handle_request(int c_fd)
{
    struct WireRequest {
        uint64_t address;
        bool     is_write;
        uint32_t data_size;
        uint8    data[64];
    } wire;

    ssize_t n = ::recv(c_fd, &wire, sizeof(wire), MSG_WAITALL);
    if (n < 0)
        throw DRAMError("failed to recieve memory access request from client (connection id: " + std::to_string(c_fd) + ")");

    uint64 seq  = next_seq.load() + 1;
    std::vector<uint8> d;
    std::copy(wire.data, wire.data + wire.data_size, d.begin());
    auto* req = new DRAMRequest(wire.address, wire.is_write, d);
    req->seq = seq;
    req->client_fd = c_fd;

    {
        std::lock_guard<std::mutex> lk(dispatcher_mtx);
        dispatcher_queue.push(req);
    }
    dispatcher_cv.notify_one();
}

uint64 DRAMController::get_bank_id(uint64 address)
{
    return dram_config.extract(address, dram_config.address_mappings.bank_mask);
}

uint64 DRAMController::get_row_id(uint64 address)
{
    return dram_config.extract(address, dram_config.address_mappings.row_mask);
}

uint64 DRAMController::get_col_id(uint64 address)
{
    return dram_config.extract(address, dram_config.address_mappings.col_mask);
}

} // namespace dram
