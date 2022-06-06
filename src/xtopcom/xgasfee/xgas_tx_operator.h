// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address.h"
#include "xdata/xcons_transaction.h"

namespace top {
namespace gasfee {

class xtop_gas_tx_operator {
public:
    explicit xtop_gas_tx_operator(xobject_ptr_t<data::xcons_transaction_t> const & tx);
    xtop_gas_tx_operator(xtop_gas_tx_operator const &) = delete;
    xtop_gas_tx_operator & operator=(xtop_gas_tx_operator const &) = delete;
    xtop_gas_tx_operator(xtop_gas_tx_operator &&) = default;
    xtop_gas_tx_operator & operator=(xtop_gas_tx_operator &&) = default;
    ~xtop_gas_tx_operator() = default;

public:
    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;
    std::string sender_str() const;
    std::string recver_str() const;
    data::enum_xtransaction_type tx_type() const;
    base::enum_transaction_subtype tx_subtype() const;
    data::enum_xtransaction_version tx_version() const;
    evm_common::u256 tx_eth_gas_limit() const;
    evm_common::u256 tx_eth_fee_per_gas() const;
    evm_common::u256 tx_eth_limited_gasfee() const;
    uint64_t deposit() const;
    uint64_t tx_last_action_used_deposit() const;

    uint64_t tx_fixed_tgas() const;
    uint64_t tx_bandwith_tgas() const;
    uint64_t tx_disk_tgas() const;

    uint64_t balance_to_tgas(const uint64_t balance) const;
    uint64_t tgas_to_balance(const uint64_t tgas) const;
    evm_common::u256 wei_to_utop(const evm_common::u256 wei) const;
    evm_common::u256 utop_to_wei(const evm_common::u256 utop) const;

    bool is_one_stage_tx() const;

private:
    xobject_ptr_t<data::xcons_transaction_t> m_tx{nullptr};
};
using xgas_tx_operator_t = xtop_gas_tx_operator;

}  // namespace gasfee
}  // namespace top
