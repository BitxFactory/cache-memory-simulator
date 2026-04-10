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

#include <stdexcept>

#include "dram_config.hpp"

namespace dram
{

// Implementation of DRAMConfig methods

DRAMConfig::DRAMConfig(SystemConfig& config, uint8 chip_width, AddressMapping& address_mappings)
: sys_config(config), chip_width_bits(chip_width), address_mappings(address_mappings),
    bank_bits(0), row_bits(0), col_bits(0)
{
    // calculate bits from address mappings
    bank_bits = popcount(address_mappings.bank_mask);
    row_bits = popcount(address_mappings.row_mask);
    col_bits = popcount(address_mappings.col_mask);

    std::string err_msg;
    if (!validate(&err_msg)) {
        throw std::invalid_argument("DRAMConfig validation failed: " + err_msg);
    }
}

bool DRAMConfig::validate(std::string* err) const {
    auto fail = [&](const char* msg) -> bool {
        if (err) *err = msg;
        return false;
    };

    // Validate system config first
    if (!sys_config.validate(err)) {
        return false;
    }

    // chip_width_bits validations
    if (chip_width_bits == 0) {
        return fail("chip_width_bits must be > 0");
    }

    if (!is_power_of_2(chip_width_bits)) {
        return fail("chip_width_bits must be power of 2");
    }

    if (chip_width_bits > sys_config.data_bus_width) {
        return fail("chip_width_bits cannot exceed data_bus_width");
    }

    if (sys_config.data_bus_width % chip_width_bits != 0) {
        return fail("data_bus_width must be a multiple of chip_width_bits");
    }

    // Ensure number_of_chips is valid
    if (number_of_chips() == 0) {
        return fail("number_of_chips calculation resulted in 0");
    }

    // Address space evenly divides by chip count
    if (sys_config.address_space_bytes() % number_of_chips() != 0) {
        return fail("address space doesn't divide evenly by number of chips");
    }

    // bank_bits validations
    if (bank_bits == 0 || bank_bits > 63) {
        return fail("bank_bits must be in range [1, 63]");
    }

    // row/col bits validations
    if (row_bits == 0 || row_bits > 63) {
        return fail("row_bits must be in range [1, 63]");
    }

    if (col_bits == 0 || col_bits > 63) {
        return fail("col_bits must be in range [1, 63]");
    }

    // Overflow prevention: row + col must be < 64 for shift operations
    if (row_bits + col_bits >= 64) {
        return fail("row_bits + col_bits must be < 64 (shift overflow)");
    }

    // Address partition integrity
    if (bank_bits + row_bits + col_bits != sys_config.address_bus_width) {
        return fail("bank_bits + row_bits + col_bits must equal address_bus_width");
    }

    // Chip capacity sanity check
    if (size_of_chip_bytes() < (uint64)number_of_banks() * size_of_subarray_bytes()) {
        return fail("chip capacity < (banks × subarray size)");
    }

    return true;
}

uint64 DRAMConfig::extract(uint64 address, uint64 mask)
{
    uint64 d = first_one_position(mask);
    return (address & mask) / d;
}

uint8 DRAMConfig::popcount(uint64 mask)
{
    uint8 count = 0;
    while (mask) {
        mask &= mask - 1;  // clears lowest set bit
        count++;
    }
    return count;
}

uint8 DRAMConfig::first_one_position(uint64 mask)
{
    assert(mask > 0 &&
            "mask must be positive integer");

    uint8 pos = 0;
    while (mask % 2 == 0) {
        pos++;
        mask /= 2;
    }

    return pos;
}

constexpr uint32 DRAMConfig::number_of_chips() const {
    return sys_config.data_bus_width / chip_width_bits;
}

constexpr uint64 DRAMConfig::size_of_chip_bytes() const {
    return sys_config.address_space_bytes() / (uint64)number_of_chips();
}

constexpr uint32 DRAMConfig::number_of_banks() const {
    return (1LL << bank_bits);
}

constexpr uint32 DRAMConfig::number_of_subarrays() const {
    return chip_width_bits;
}

constexpr uint64 DRAMConfig::size_of_subarray_bytes() const {
    return ((1LL << row_bits) * (1LL << col_bits)) / 8;
}

} // namespace dram