// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xeth_address.h"

#include "xbasic/xerror/xerror.h"
#include "xbasic/xhex.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xerror/xerror.h"

#include <algorithm>
#include <cassert>

NS_BEG2(top, common)

xtop_eth_address xtop_eth_address::build_from(xaccount_address_t const & account_address, std::error_code & ec) {
    if (account_address.type() != base::enum_vaccount_addr_type_secp256k1_eth_user_account && account_address.type() != base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
        ec = common::error::xerrc_t::invalid_account_type;
        return {};
    }

    auto const account_string = account_address.value().substr(6);
    assert(account_string.size() == 40);
    return xeth_address_t{account_string, ec};
}

xtop_eth_address xtop_eth_address::build_from(xaccount_address_t const & account_address) {
    std::error_code ec;
    auto ret = xtop_eth_address::build_from(account_address, ec);
    top::error::throw_error(ec);

    return ret;
}

xtop_eth_address xtop_eth_address::build_from(std::array<uint8_t, 20> const & address_data) {
    return xeth_address_t{address_data};
}

xtop_eth_address xtop_eth_address::build_from(xbytes_t const & address_data, std::error_code & ec) {
    if (address_data.size() != xtop_eth_address::size()) {
        ec = common::error::xerrc_t::invalid_account_address;
        return {};
    }

    std::array<uint8_t, xtop_eth_address::size()> addr_data;
    std::copy_n(std::begin(address_data), xtop_eth_address::size(), std::begin(addr_data));
    return xtop_eth_address(addr_data);
}

xtop_eth_address xtop_eth_address::build_from(xbytes_t const & address_data) {
    std::error_code ec;
    auto ret = xtop_eth_address::build_from(address_data, ec);
    top::error::throw_error(ec);

    return ret;
}

xtop_eth_address::xtop_eth_address() {
    std::fill(std::begin(raw_address_), std::end(raw_address_), 0);
}

xtop_eth_address::xtop_eth_address(std::array<uint8_t, 20> const & raw_account_address) : raw_address_(raw_account_address) {
}

xtop_eth_address::xtop_eth_address(std::string const & account_string) {
    std::error_code ec;
    auto const & bytes = top::from_hex(account_string, ec);
    top::error::throw_error(ec);

    assert(bytes.size() == raw_address_.size());

    std::copy(std::begin(bytes), std::end(bytes), std::begin(raw_address_));
}

xtop_eth_address::xtop_eth_address(std::string const & account_string, std::error_code & ec) {
    auto const & bytes = top::from_hex(account_string, ec);
    assert(bytes.size() == raw_address_.size());

    std::copy(std::begin(bytes), std::end(bytes), std::begin(raw_address_));
}

std::string xtop_eth_address::to_hex_string() const {
    return top::to_hex_prefixed(raw_address_);
}

xbytes_t xtop_eth_address::to_bytes() const {
    return {std::begin(raw_address_), std::end(raw_address_)};
}

xbytes_t xtop_eth_address::to_h160() const {
    return to_bytes();
}

xbytes_t xtop_eth_address::to_h256() const {
    xbytes_t h256(32, 0);
    xbytes_t h160 = to_h160();
    std::copy_n(std::begin(h160), h160.size(), std::next(std::begin(h256), 12));
    return h256;
}

xtop_eth_address const & xtop_eth_address::zero() {
    static xtop_eth_address const z;
    return z;
}

NS_END2
