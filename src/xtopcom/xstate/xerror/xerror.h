// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <system_error>
#include <type_traits>

NS_BEG3(top, state, error)

enum class xenum_errc : uint16_t {
    ok,
    token_insufficient,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const ec) noexcept;
std::error_condition make_error_condition(xerrc_t const ec) noexcept;

std::error_category const & state_category();

NS_END3

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::state::error::xerrc_t> final {
    size_t operator()(top::state::error::xerrc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::state::error::xerrc_t> : std::true_type {
};

template <>
struct is_error_condition_enum<top::state::error::xerrc_t> : std::true_type {
};

NS_END1
