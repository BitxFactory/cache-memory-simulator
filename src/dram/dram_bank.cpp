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

#include <dram_bank.hpp>
#include <common/error.hpp>

#include <string>

namespace dram
{

DRAMBank::DRAMBank(uint32 id, DRAMConfig& config): id(id), dram_config(config)
{
    arrays.resize(dram_config.number_of_subarrays());

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

void DRAMBank::transfer_data(uint32 col_addr, bool is_write, std::span<uint8> data)
{
    // read/write (t)
    for (uint32 i = 0; i < arrays.size(); i++) {
        if (is_write) {
            if (!arrays[i]->write(col_addr, data[i])) {
                throw DRAMError("Failed to write to DRAM subarray " + std::to_string(i + 1) + " at column " + std::to_string(col_addr));
            }
        } else {
            if (!arrays[i]->read(col_addr, data[i])) {
                throw DRAMError("Failed to read from DRAM subarray " + std::to_string(i + 1) + " at column " + std::to_string(col_addr));
            }
        }
    }
}

DRAMBankEqClass::DRAMBankEqClass(uint32 id, DRAMConfig& config)
    : id(id), dram_config(config)
{
    banks.resize(dram_config.number_of_chips());
    for (uint32 i = 0; i < dram_config.number_of_chips(); i++) {
        banks[i] = std::make_unique<DRAMBank>(i + 1, dram_config);
    }
}

void DRAMBankEqClass::open_row(uint32 row_addr)
{
    for (uint32 i = 0; banks.size(); i++) {
        banks[i]->open_row(row_addr);
    }
}

void DRAMBankEqClass::transfer_data(uint32 col_addr, bool is_write, std::span<uint8> data)
{
    uint8 bus_width = dram_config.sys_config.data_bus_width;
    uint8 chip_bits = dram_config.chip_width_bits;
    uint32 num_of_banks = banks.size();

    if (data.size() != bus_width) {
        throw DRAMError("data size mismatch, must be equal to data bus width");
    }

    if (num_of_banks != (bus_width / chip_bits)) {
        throw DRAMError("number of banks mismatch in an equivalence class");
    }

    for (uint32 i = 0; i < num_of_banks; i++) {
        banks[i]->transfer_data(col_addr, is_write, data.subspan(i * chip_bits, chip_bits));
    }
}

} // namespace dram
