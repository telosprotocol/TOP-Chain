// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address.h"
#include "xcontract_runtime/xaction_session.h"
#include "xdata/xconsensus_action_fwd.h"
#include "xevm_contract_runtime/xevm_type.h"
#include "xstate_accessor/xstate_accessor.h"

NS_BEG2(top, contract_runtime)

template <>
class xtop_action_session<data::xevm_consensus_action_t> {
private:
    observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> m_associated_runtime;
    observer_ptr<evm_runtime::xevm_state_t> m_evm_state;

public:
    xtop_action_session(xtop_action_session const &) = delete;
    xtop_action_session & operator=(xtop_action_session const &) = delete;
    xtop_action_session(xtop_action_session &&) = default;
    xtop_action_session & operator=(xtop_action_session &&) = default;
    ~xtop_action_session() = default;

    xtop_action_session(observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> associated_runtime, observer_ptr<evm_runtime::xevm_state_t> evm_state) noexcept;

    xtransaction_execution_result_t execute_action(std::unique_ptr<data::xbasic_top_action_t const> action);
};

NS_END2