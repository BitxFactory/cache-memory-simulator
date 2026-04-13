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

#include <cerrno>
#include <cstring>
#include <format>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <dram.hpp>

namespace dram
{

int DRAM::setup_client()
{
    int client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1)
        throw std::runtime_error("socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (socket_path.size() >= sizeof(addr.sun_path))
        throw std::runtime_error("Socket path too long");

    std::memset(addr.sun_path, 0, sizeof(addr.sun_path));
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(client_fd,
                  reinterpret_cast<sockaddr*>(&addr),
                  sizeof(addr)) < 0) {
        int err = errno;
        ::close(client_fd);
        throw std::runtime_error(std::string("connect() failed: ") + std::strerror(err));
    }

    return client_fd;
}

DRAM::DRAM(DRAMConfig& config, std::string socket_path)
    : dram_config(config), socket_path(socket_path) {}

void DRAM::access(uint32 address, bool is_write, std::vector<uint8>& data)
{
    // connect to the socket
    auto c_fd = setup_client();

    // send the request to the server
    struct WireRequest {
        uint64_t address;
        bool     is_write;
        uint32_t data_size;
        uint8    data[64];
    } wire;

    wire.address = address;
    wire.is_write = is_write;
    wire.data_size = data.size();
    std::copy(data.begin(), data.end(), &wire.data[0]);

    ssize_t n = ::send(c_fd, &wire, sizeof(wire), MSG_WAITALL);
    if (n < 0)
        throw DRAMError("failed to send memory access request to address: " + std::format("0x{:08x}", address));

    // wait for response
    DRAMResponse resp;
    n = ::recv(c_fd, &resp, sizeof(resp), MSG_WAITALL);

    if (n < 0 || !resp.success)
        throw DRAMError("error recieving response from the controller");

    // read the data from response
    if (!is_write)
        std::copy(resp.data.begin(), resp.data.end(), data.begin());
    
    ::close(c_fd);
}

} // namespace dram
