// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xeth_address_fwd.h"
#include "xbasic/xbyte_buffer.h"

#include <array>
#include <cstdint>
#include <string>
#include <system_error>

NS_BEG2(top, common)

class xtop_eth_address {
private:
    static constexpr size_t SIZE{20};
    std::array<uint8_t, SIZE> raw_address_;

public:
    xtop_eth_address() = default;
    xtop_eth_address(xtop_eth_address const &) = default;
    xtop_eth_address & operator=(xtop_eth_address const &) = default;
    xtop_eth_address(xtop_eth_address &&) = default;
    xtop_eth_address & operator=(xtop_eth_address &&) = default;
    ~xtop_eth_address() = default;

    explicit xtop_eth_address(std::array<uint8_t, SIZE> const & raw_account_address);

    static xtop_eth_address build_from(xaccount_address_t const & account_address, std::error_code & ec);
    static xtop_eth_address build_from(xaccount_address_t const & account_address);
    static xtop_eth_address build_from(std::array<uint8_t, 20> const & address_data);
    static xtop_eth_address build_from(xbytes_t const & address_data, std::error_code & ec);
    static xtop_eth_address build_from(xbytes_t const & address_data);

private:
    explicit xtop_eth_address(std::string const & account_string);
    explicit xtop_eth_address(std::string const & account_string, std::error_code & ec);

public:
    constexpr static size_t size() noexcept {
        return SIZE;
    }

    std::string to_hex_string() const;
    xbytes_t to_bytes() const;
    xbytes_t to_h160() const;
    xbytes_t to_h256() const;
};

NS_END2
