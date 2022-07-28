// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xdata/xcrosschain_whitelist.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

#ifdef ETH_BRIDGE_TEST
#include <fstream>
#endif

NS_BEG3(top, contract_runtime, evm)

using evm_common::bigint;
using evm_common::h128;
using evm_common::h256;
using evm_common::h512;
using evm_common::u256;
using evm_common::u64;
using evm_common::eth::xeth_header_t;

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

template <typename T>
class xtop_evm_crosschain_syscontract_face : public xtop_evm_syscontract_face {
public:
    xtop_evm_crosschain_syscontract_face() {
        m_whitelist = load_whitelist();
        if (m_whitelist.empty()) {
            xwarn("[xtop_evm_eth_bridge_contract::xtop_evm_eth_bridge_contract] m_whitelist empty!");
        }
    }
    xtop_evm_crosschain_syscontract_face(xtop_evm_crosschain_syscontract_face const &) = delete;
    xtop_evm_crosschain_syscontract_face & operator=(xtop_evm_crosschain_syscontract_face const &) = delete;
    xtop_evm_crosschain_syscontract_face(xtop_evm_crosschain_syscontract_face &&) = default;
    xtop_evm_crosschain_syscontract_face & operator=(xtop_evm_crosschain_syscontract_face &&) = default;
    ~xtop_evm_crosschain_syscontract_face() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;

    std::set<std::string> load_whitelist();

    bool init(const xbytes_t & rlp_bytes) {
        return static_cast<T*>(this)->init(rlp_bytes);
    }
    bool sync(const xbytes_t & rlp_bytes) {
        return static_cast<T*>(this)->sync(rlp_bytes);
    }
    bool is_known(const u256 height, const xbytes_t & hash_bytes) const {
        return static_cast<const T*>(this)->is_known(height, hash_bytes);
    }
    bool is_confirmed(const u256 height, const xbytes_t & hash_bytes) const {
        return static_cast<const T*>(this)->is_confirmed(height, hash_bytes);
    }
    bigint get_height() const {
        return static_cast<const T*>(this)->get_height();
    }
    void reset() {
        return static_cast<T*>(this)->reset();
    }

    std::shared_ptr<data::xunit_bstate_t> m_contract_state{nullptr};
    std::set<std::string> m_whitelist;
};

template <typename T>
inline bool xtop_evm_crosschain_syscontract_face<T>::execute(xbytes_t input,
                                                             uint64_t target_gas,
                                                             sys_contract_context const & context,
                                                             bool is_static,
                                                             observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                                             sys_contract_precompile_output & output,
                                                             sys_contract_precompile_error & err) {
    // method ids:
    //--------------------------------------------------
    // init(bytes,string)                    => 6158600d
    // sync(bytes)                           => 7eefcfa2
    // get_height()                          => b15ad2e8
    // is_known(uint256,bytes32)             => 6d571daf
    // is_confirmed(uint256,bytes32)         => d398572f
    // reset()                               => d826f88f
    //--------------------------------------------------
    constexpr uint32_t method_id_init{0x6158600d};
    constexpr uint32_t method_id_sync{0x7eefcfa2};
    constexpr uint32_t method_id_get_height{0xb15ad2e8};
    constexpr uint32_t method_id_is_known{0x6d571daf};
    constexpr uint32_t method_id_is_confirmed{0xd398572f};
    constexpr uint32_t method_id_reset{0xd826f88f};

    // check param
    assert(state_ctx);
    m_contract_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.address, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
    if (m_contract_state == nullptr) {
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] state nullptr");
        return false;
    }
    if (input.empty()) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] invalid input");
        return false;
    }
    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::begin(input), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] illegal input function selector");
        return false;
    }
    xinfo("[xtop_evm_crosschain_syscontract_face::execute] caller: %s, address: %s, method_id: 0x%x, input size: %zu",
          context.caller.to_hex_string().c_str(),
          context.address.to_hex_string().c_str(),
          function_selector.method_id,
          input.size());

    switch (function_selector.method_id) {
    case method_id_init: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] init is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        if (!init(headers_rlp)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] init headers error");
            return false;
        }
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_sync: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] sync is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract error");
            return false;
        }
        if (!sync(headers_rlp)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] sync headers error");
            return false;
        }
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_get_height: {
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(get_height()));
        return true;
    }
    case method_id_is_known: {
        auto height = abi_decoder.extract<u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t known{0};
        if (is_known(height, hash_bytes)) {
            known = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(known));
        return true;
    }
    case method_id_is_confirmed: {
        auto height = abi_decoder.extract<u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t confirmed{0};
        if (is_confirmed(height, hash_bytes)) {
            confirmed = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(confirmed));
        return true;
    }
    case method_id_reset: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        reset();
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(0));
        return true;
    }
    default: {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);
        return false;
    }
    }

    return true;
}

template <typename T>
inline std::set<std::string> xtop_evm_crosschain_syscontract_face<T>::load_whitelist() {
    json j;
#ifdef ETH_BRIDGE_TEST
#    define WHITELIST_FILE "eth_bridge_whitelist.json"
    xinfo("[xtop_evm_crosschain_syscontract_face::load_whitelist] load from file %s", WHITELIST_FILE);
    std::ifstream data_file(WHITELIST_FILE);
    if (data_file.good()) {
        data_file >> j;
        data_file.close();
    } else {
        xwarn("[xtop_evm_crosschain_syscontract_face::load_whitelist] file %s open error", WHITELIST_FILE);
        return {};
    }
#else
    xinfo("[xtop_evm_crosschain_syscontract_face::load_whitelist] load from code");
    j = json::parse(data::crosschain_whitelist());
#endif

    if (!j.count("whitelist")) {
        xwarn("[xtop_evm_crosschain_syscontract_face::load_whitelist] load whitelist failed");
        return {};
    }
    std::set<std::string> ret;
    auto const & list = j["whitelist"];
    for (auto const item : list) {
        ret.insert(item.get<std::string>());
    }
    return ret;
}

NS_END3
