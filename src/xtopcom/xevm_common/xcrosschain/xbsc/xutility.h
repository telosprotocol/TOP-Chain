// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbytes.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xcrosschain/xbsc/xconfig.h"
#include "xevm_common/xcrosschain/xbsc/xvote.h"

#include <system_error>

NS_BEG4(top, evm, crosschain, bsc)

auto get_validator_bytes_from_header(evm_common::xeth_header_t const & header,
                                     xchain_config_t const & chain_config,
                                     xparlia_config_t const & parlia_config) -> xbytes_t;

auto get_vote_attestation_from_header(evm_common::xeth_header_t const & header,
                                      xchain_config_t const & chain_config,
                                      xparlia_config_t const & parlia_config,
                                      std::error_code & ec) -> xvote_attestation_t;

//auto verify_vote_attestation()

NS_END4
