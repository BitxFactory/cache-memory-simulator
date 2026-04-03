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

#include "../common/utility.hpp"
#include "../common/system_config.hpp"

namespace dram
{

/**
 * @struct AddressMapping
 * @brief address mapping
 */
struct AddressMapping {
    uint64 rank_mask;   // 0 if single rank
    uint64 bank_mask;   // 0 if single bank
    uint64 row_mask;
    uint64 col_mask;
};

/**
 * @struct DRAMConfig
 * @brief configuration of a rank of a DRAM
 */
struct DRAMConfig {
    SystemConfig& sys_config;  ///< system configuration

    uint8 chip_width_bits;     ///< total data pins in a chip

    uint8 bank_bits;           ///< bits to decode banks in a chip
    uint8 row_bits;            ///< bits to decode word lines in a bank's subarray
    uint8 col_bits;            ///< bits to decode bit lines in a bank's subarray

    AddressMapping& address_mappings;  ///< address mapping of DRAM

    /**
     * @brief DRAMConfig constructor
     */
    DRAMConfig(SystemConfig& config, uint8 chip_width, AddressMapping& address_mappings);

    /**
     * @brief number of chips in a single rank
     */
    constexpr uint32 number_of_chips() const;

    /**
     * @brief size of each chip in bytes
     */
    constexpr uint64 size_of_chip_bytes() const;

    /**
     * @brief number of banks in a chip
     */
    constexpr uint32 number_of_banks() const;

    /**
     * @brief number of subarrays in a bank
     */
    constexpr uint32 number_of_subarrays() const;

    /**
     * @brief size of a subarray in bytes
     * 
     * Assuming 1-bit-per cell
     */
    constexpr uint64 size_of_subarray_bytes() const;

    // validate
    bool validate(std::string* err = nullptr) const;

private:
    uint8 popcount(uint64 mask);
};

} // namespace dram


