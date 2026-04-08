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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <filesystem>
#include <cstring>
#include <stdexcept>

#include "dram_controller.hpp"

namespace dram
{

DRAMRequest::DRAMRequest(uint32 r, uint32 c, bool is_write, std::vector<uint8>& d)
    : row_addr(r), col_addr(c), is_write(is_write), data(d) {}

DRAMController::DRAMController(DRAMConfig& config, std::string socket_path)
    : dram_config(config), socket_path(socket_path)
{


    // create banks
    eq_class.resize(dram_config.number_of_banks());
    for (uint32 i = 0; i < dram_config.number_of_banks(); i++) {
        eq_class[i] = std::make_unique<DRAMBankEqClass>(i + 1, config);
    }
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
    
void DRAMController::run() {
    setup_server();

    while(true) {
        // accept client connection
        int client_fd = accept(controller_fd, nullptr, nullptr);


    }
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

} // namespace dram
