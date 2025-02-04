// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xmodule_type.h"

#include <system_error>

namespace top {
namespace txexecutor {
namespace error {

enum class xenum_errc {
    ok,

    xconsensus_service_error_base = chainbase::enum_xmodule_type::xmodule_type_xtxexecutor,

    xconsensus_service_error_balance_not_enough,
    xconsensus_service_error_min_deposit_error,
    xconsensus_service_error_action_not_valid,
    xconsensus_service_error_addr_type_error,
    xconsensus_service_error_sign_error,
    xtransaction_parse_type_invalid,
    xchain_error_action_parse_exception,
    xunit_contract_exec_no_property_change,
    xtransaction_confirm_state_unchange,

    xtransaction_pledge_token_overflow,
    xtransaction_not_enough_pledge_token_tgas,
    xtransaction_not_enough_pledge_token_disk,
    xtransaction_over_half_total_pledge_token,
    xtransaction_not_enough_deposit,
    xtransaction_too_much_deposit,
    xtransaction_not_enough_token,
    xtransaction_non_positive_pledge_token,
    xtransaction_non_top_token_illegal,
    xtransaction_non_zero_frozen_tgas,
    xtransaction_pledge_too_much_token,
    xtransaction_contract_pledge_token_illegal,
    xtransaction_contract_pledge_token_target_illegal,
    xtransaction_contract_pledge_token_source_target_mismatch,
    xtransaction_contract_not_enough_tgas,
    xtransaction_pledge_token_source_target_type_mismatch,
    xtransaction_pledge_contract_type_illegal,
    xtransaction_pledge_contract_owner_sign_err,
    xtransaction_pledge_redeem_vote_err,

    enum_xtxexecutor_error_tx_nonce_not_match,
    enum_xtxexecutor_error_load_state_fail,
    enum_xtxexecutor_error_execute_fail,
    enum_xtxexecutor_error_tx_receiptid_not_match,

    xconsensus_service_error_max,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t errc) noexcept;
std::error_condition make_error_condition(xerrc_t errc) noexcept;

std::error_category const & txexecutor_category();

}
}
}

namespace std {

#if !defined(XCXX14)

template <>
struct hash<top::txexecutor::error::xerrc_t> final {
    size_t operator()(top::txexecutor::error::xerrc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::txexecutor::error::xerrc_t> : std::true_type {
};

template <>
struct is_error_condition_enum<top::txexecutor::error::xerrc_t> : std::true_type {
};

}
