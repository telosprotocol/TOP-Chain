// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_erc20_contract.h"

#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xnative_contract_address.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xfixed_hash.h"
#include "xevm_contract_runtime/xerror/xerror.h"
#include "xevm_runner/evm_engine_interface.h"

#include <cinttypes>

NS_BEG4(top, contract_runtime, evm, sys_contract)

static constexpr char const * event_hex_string_transfer{"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef"};
static constexpr char const * event_hex_string_approve{"0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925"};
static constexpr char const * event_hex_string_ownership_transferred{"0x8be0079c531659141344cd1fd0a4f28419497f9722a3daafe3b4186f6b6457e0"}; // OwnershipTransferred(address indexed oldOwner, address indexed newOwner)
static constexpr char const * event_hex_string_controller_set{"0xea4ba01507e54b7b5990927a832da3ab6a71e981b5d53ffad97def0f85fcfb20"};        // ControllerSet(address indexed oldController, address indexed newController)

bool xtop_evm_erc20_sys_contract::execute(xbytes_t input,
                                          uint64_t target_gas,
                                          sys_contract_context const & context,
                                          bool is_static,
                                          observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                          sys_contract_precompile_output & output,
                                          sys_contract_precompile_error & err) {
    // ERC20 method ids:
    //--------------------------------------------------
    // decimals()                            => 0x313ce567
    // totalSupply()                         => 0x18160ddd
    // balanceOf(address)                    => 0x70a08231
    // transfer(address,uint256)             => 0xa9059cbb
    // transferFrom(address,address,uint256) => 0x23b872dd
    // approve(address,uint256)              => 0x095ea7b3
    // allowance(address,address)            => 0xdd62ed3e
    // mint(address,uint256)                 => 0x40c10f19
    // burnFrom(address,uint256)             => 0x79cc6790
    // transferOwnership(address)            => 0xf2fde38b
    // setController(address)                => 0x92eefe9b
    //--------------------------------------------------
    constexpr uint32_t method_id_decimals{0x313ce567};
    constexpr uint32_t method_id_total_supply{0x18160ddd};
    constexpr uint32_t method_id_balance_of{0x70a08231};
    constexpr uint32_t method_id_transfer{0xa9059cbb};
    constexpr uint32_t method_id_transfer_from{0x23b872dd};
    constexpr uint32_t method_id_approve{0x095ea7b3};
    constexpr uint32_t method_id_allowance{0xdd62ed3e};
    constexpr uint32_t method_id_mint{0x40c10f19};
    constexpr uint32_t method_id_burn_from{0x79cc6790};
    constexpr uint32_t method_id_transfer_ownership{0xf2fde38b};
    constexpr uint32_t method_id_set_controller{0x92eefe9b};

    assert(state_ctx);

    // erc20_uuid (1 byte) | erc20_method_id (4 bytes) | parameters (depends)
    if (input.empty()) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("predefined erc20 contract: invalid input");

        return false;
    }

    common::xtoken_id_t const erc20_token_id{top::from_byte<common::xtoken_id_t>(input.front())};
    if (erc20_token_id != common::xtoken_id_t::top && erc20_token_id != common::xtoken_id_t::usdc && erc20_token_id != common::xtoken_id_t::usdt &&
        erc20_token_id != common::xtoken_id_t::eth) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

        xwarn("predefined erc20 contract: not supported token: %d", static_cast<int>(erc20_token_id));

        return false;
    }

    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::next(std::begin(input), 1), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("predefined erc20 contract: illegal input data");

        return false;
    }

    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

        xwarn("predefined erc20 contract: illegal input function selector");

        return false;
    }

    switch (function_selector.method_id) {
    case method_id_decimals: {
        xdbg("predefined erc20 contract: decimals");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: decimals not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        evm_common::u256 decimal{6};
        output.exit_status = Returned;
        output.cost = 0;
        output.output = top::to_bytes(decimal);

        return true;
    }

    case method_id_total_supply: {
        xdbg("predefined erc20 contract: totalSupply");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: totalSupply not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        uint64_t const total_supply_gas_cost = 2538;
        if (target_gas < total_supply_gas_cost) {
            err.fail_status = Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: totalSupply out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, total_supply_gas_cost);

            return false;
        }

        if (!abi_decoder.empty()) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: totalSupply with non-empty parameter");

            return false;
        }

        evm_common::u256 supply{0};
        switch (erc20_token_id) {
        case common::xtoken_id_t::usdt:
            supply = 36815220940394632;
            break;

        case common::xtoken_id_t::usdc:
            supply = 45257057549529550;
            break;

        case common::xtoken_id_t::top:
            supply = 20000000000000000;
            break;

        default:
            assert(false);
            break;
        }

        output.exit_status = Returned;
        output.cost = total_supply_gas_cost;
        output.output = top::to_bytes(supply);

        return true;
    }

    case method_id_balance_of: {
        xdbg("predefined erc20 contract: balanceOf");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: balanceOf not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        uint64_t const balance_of_gas_cost = 3268;
        if (target_gas < balance_of_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: balanceOf out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, balance_of_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 1) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: balance_of with invalid parameter (parameter count not one)");

            return false;
        }

        common::xeth_address_t const eth_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: balance_of invalid account");

            return false;
        }

        auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(eth_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());

        evm_common::u256 value{0};
        switch (erc20_token_id) {
        case common::xtoken_id_t::top: {
            value = state->balance();
            break;
        }

        case common::xtoken_id_t::usdt:
            XATTRIBUTE_FALLTHROUGH;
        case common::xtoken_id_t::usdc: {
            value = state->tep_token_balance(erc20_token_id);
            break;
        }

        default:
            assert(false);  // won't reach.
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: balance_of invalid token id %d", static_cast<int>(erc20_token_id));

            return false;
        }

        auto result = top::to_bytes(value);
        assert(result.size() == 32);

        output.cost = balance_of_gas_cost;
        output.exit_status = Returned;
        output.output = top::to_bytes(value);

        return true;
    }

    case method_id_transfer: {
        xdbg("predefined erc20 contract: transfer");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: transfer not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        uint64_t const transfer_gas_cost = 18446;
        uint64_t const transfer_reverted_gas_cost = 3662;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transfer is not allowed in static context");

            return false;
        }

        if (target_gas < transfer_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: transfer out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, transfer_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transfer with invalid parameter");

            return false;
        }

        common::xeth_address_t recipient_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transfer with invalid account");

            return false;
        }
        common::xaccount_address_t recipient_account_address = common::xaccount_address_t::build_from(recipient_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transfer with invalid value");

            return false;
        }

        auto sender_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
        auto recver_state = state_ctx->load_unit_state(recipient_account_address.vaccount());

        std::error_code ec;
        sender_state->transfer(erc20_token_id, top::make_observer(recver_state.get()), value, ec);

        if (!ec) {
            auto const & contract_address = context.address;
            auto const & caller_address = context.caller;

            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_transfer));
            topics.push_back(evm_common::xh256_t(caller_address.to_h256()));
            topics.push_back(evm_common::xh256_t(recipient_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));

            result[31] = 1;

            output.cost = transfer_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_reverted_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transfer reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_transfer_from: {
        xdbg("predefined erc20 contract: transferFrom");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: transferFrom not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        uint64_t const transfer_from_gas_cost = 18190;
        uint64_t const transfer_from_reverted_gas_cost = 4326;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_from_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transferFrom is not allowed in static context");

            return false;
        }

        if (target_gas < transfer_from_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: transferFrom out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, transfer_from_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 3) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferFrom with invalid parameters");

            return false;
        }

        std::error_code ec;
        common::xeth_address_t owner_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferFrom invalid owner account");

            return false;
        }

        common::xeth_address_t recipient_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferFrom invalid recipient account");

            return false;
        }

        common::xaccount_address_t owner_account_address = common::xaccount_address_t::build_from(owner_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        common::xaccount_address_t recipient_account_address = common::xaccount_address_t::build_from(recipient_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferFrom invalid value");

            return false;
        }

        auto owner_state = state_ctx->load_unit_state(owner_account_address.vaccount());
        owner_state->update_allowance(erc20_token_id,
                                      common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account),
                                      value,
                                      data::xallowance_update_op_t::decrease,
                                      ec);
        if (!ec) {
            auto recver_state = state_ctx->load_unit_state(recipient_account_address.vaccount());
            owner_state->transfer(erc20_token_id, top::make_observer(recver_state.get()), value, ec);
            if (!ec) {
                result[31] = 1;
            }
        }

        if (!ec) {
            auto const & contract_address = context.address;

            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_transfer));
            topics.push_back(evm_common::xh256_t(owner_address.to_h256()));
            topics.push_back(evm_common::xh256_t(recipient_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));

            result[31] = 1;

            output.cost = transfer_from_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_from_reverted_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transferFrom reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_approve: {
        xdbg("predefined erc20 contract: approve");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: approve not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        uint64_t const approve_gas_cost = 18599;
        xbytes_t result(32, 0);
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = approve_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: approve is not allowed in static context");

            return false;
        }

        if (target_gas < approve_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: approve out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, approve_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: approve with invalid parameter");

            return false;
        }

        std::error_code ec;
        common::xeth_address_t spender_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: approve invalid spender account");

            return false;
        }
        common::xaccount_address_t spender_account_address = common::xaccount_address_t::build_from(spender_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 amount = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: approve invalid value");

            return false;
        }

        auto sender_state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
        sender_state->approve(erc20_token_id, spender_account_address, amount, ec);

        auto const & contract_address = context.address;
        auto const & caller_address = context.caller;

        if (!ec) {
            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_approve));
            topics.push_back(evm_common::xh256_t(caller_address.to_h256()));
            topics.push_back(evm_common::xh256_t(spender_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(amount));
            result[31] = 1;

            output.cost = approve_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = approve_gas_cost / 2;
            err.output = result;

            xerror("predefined erc20 contract: approve reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return true;
    }

    case method_id_allowance: {
        xdbg("predefined erc20 contract: allowance");

        if (erc20_token_id == common::xtoken_id_t::eth) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: allowance not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        uint64_t const allowance_gas_cost = 3987;
        if (target_gas < allowance_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: allowance out of gas. gas remained %" PRIu64 " gas required %" PRIu64, target_gas, allowance_gas_cost);

            return false;
        }

        xbytes_t result(32, 0);
        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: allowance with invalid parameter");

            return false;
        }

        std::error_code ec;
        common::xeth_address_t owner_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: allowance invalid owner account");

            return false;
        }

        common::xeth_address_t spender_address = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: allowance invalid spender account");

            return false;
        }

        common::xaccount_address_t owner_account_address = common::xaccount_address_t::build_from(owner_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        common::xaccount_address_t spender_account_address = common::xaccount_address_t::build_from(spender_address, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        auto owner_state = state_ctx->load_unit_state(owner_account_address.vaccount());
        result = top::to_bytes(owner_state->allowance(erc20_token_id, spender_account_address, ec));
        assert(result.size() == 32);

        output.cost = allowance_gas_cost;
        output.output = result;
        output.exit_status = Returned;

        return true;
    }

    case method_id_mint: {
        xdbg("predefined erc20 contract: mint");

        uint64_t const mint_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = mint_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: mint is not allowed in static context");

            return false;
        }

        if (erc20_token_id == common::xtoken_id_t::top) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: mint not supported TOP token");

            return false;
        }

        // Only controller can mint tokens.
        auto const & contract_state = state_ctx->load_unit_state(evm_erc20_contract_address.vaccount());
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_controller = contract_state->tep_token_controller(erc20_token_id);
        if (msg_sender != token_controller) {
            //err.fail_status = precompile_error::Fatal;
            //err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            //xwarn("predefined erc20 contract: mint called by non-admin account %s", context.caller.c_str());

            //return false;
        }

        if (target_gas < mint_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: mint out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, mint_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: mint with invalid parameter");

            return false;
        }

        auto const recver = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: mint with invalid receiver address");

            return false;
        }
        auto const recver_address = common::xaccount_address_t::build_from(recver, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: mint with invalid value");

            return false;
        }

        if (erc20_token_id == common::xtoken_id_t::eth) {
            auto * engine_ptr = ::evm_engine();
            auto * executor_ptr = ::evm_executor();
            assert(engine_ptr != nullptr && executor_ptr != nullptr);
            if (!unsafe_eth_mint(engine_ptr, executor_ptr, recver.data(), recver.size(), value.str().c_str())) {
                xwarn("mint eth failed");
                ec = evm_runtime::error::xerrc_t::precompiled_contract_erc20_burn;
            }
        } else {
            auto recver_state = state_ctx->load_unit_state(recver_address.vaccount());
            recver_state->tep_token_deposit(erc20_token_id, value);
        }

        if (!ec) {
            auto const & contract_address = context.address;
            auto const & recipient_address = recver;

            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_transfer));
            topics.push_back(evm_common::xh256_t(common::xeth_address_t::zero().to_h256()));
            topics.push_back(evm_common::xh256_t(recipient_address.to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));
            result[31] = 1;

            output.cost = mint_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = mint_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: mint reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_burn_from: {
        xdbg("predefined erc20 contract: burnFrom");

        uint64_t const burn_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = burn_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: burnFrom is not allowed in static context");

            return false;
        }

        if (erc20_token_id == common::xtoken_id_t::top) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: burnFrom not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        auto const & contract_state = state_ctx->load_unit_state(evm_erc20_contract_address.vaccount());
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_controller = contract_state->tep_token_controller(erc20_token_id);
        if (msg_sender != token_controller) {
            //err.fail_status = precompile_error::Fatal;
            //err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            //xwarn("predefined erc20 contract: burnFrom called by non-admin account %s", context.caller.c_str());

            //return false;
        }

        if (target_gas < burn_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: burnFrom out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, burn_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 2) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: burnFrom with invalid parameter");

            return false;
        }

        common::xeth_address_t burn_from = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: burnFrom with invalid burn from address");

            return false;
        }
        common::xaccount_address_t burn_from_address = common::xaccount_address_t::build_from(burn_from, base::enum_vaccount_addr_type_secp256k1_evm_user_account);

        evm_common::u256 const value = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: burnFrom with invalid value");

            return false;
        }

        if (erc20_token_id == common::xtoken_id_t::eth) {
            auto * engine_ptr = ::evm_engine();
            auto * executor_ptr = ::evm_executor();
            assert(engine_ptr != nullptr && executor_ptr != nullptr);
            if (!unsafe_eth_burn(engine_ptr, executor_ptr, burn_from.data(), burn_from.size(), value.str().c_str())) {
                xwarn("burn eth failed");
                ec = evm_runtime::error::xerrc_t::precompiled_contract_erc20_burn;
            }
        } else {
            auto recver_state = state_ctx->load_unit_state(burn_from_address.vaccount());
            recver_state->tep_token_withdraw(erc20_token_id, value);
        }

        if (!ec) {
            auto const & contract_address = context.address;

            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_transfer));
            topics.push_back(evm_common::xh256_t(burn_from.to_h256()));
            topics.push_back(evm_common::xh256_t(common::xeth_address_t::zero().to_h256()));
            evm_common::xevm_log_t log(contract_address, topics, top::to_bytes(value));
            result[31] = 1;

            output.cost = burn_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(log);
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = burn_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: burnFrom reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_transfer_ownership: {
        xdbg("predefined erc20 contract: transferOwnership");

        uint64_t const transfer_ownership_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_ownership_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transferOwnership is not allowed in static context");

            return false;
        }

        if (erc20_token_id == common::xtoken_id_t::top) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: transferOwnership not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        assert(erc20_token_id == common::xtoken_id_t::eth || erc20_token_id == common::xtoken_id_t::usdc || erc20_token_id == common::xtoken_id_t::usdt);

        auto contract_state = state_ctx->load_unit_state(evm_erc20_contract_address.vaccount());
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_owner = contract_state->tep_token_owner(erc20_token_id);
        if (msg_sender != token_owner) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferOwnership called by non-admin account %s", context.caller.c_str());

            return false;
        }
        ec.clear();

        if (target_gas < transfer_ownership_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: transferOwnership out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, transfer_ownership_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 1) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferOwnership with invalid parameter");

            return false;
        }

        auto const new_owner = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: transferOwnership with invalid burn from address");

            return false;
        }

        if (new_owner.empty()) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_ownership_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transferOwnership reverted. new owner is zero");

            return false;
        }

        contract_state->tep_token_owner(erc20_token_id, common::xaccount_address_t::build_from(new_owner, base::enum_vaccount_addr_type_secp256k1_evm_user_account), ec);
        if (!ec) {
            auto const & contract_address = context.address;

            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_ownership_transferred));
            topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
            topics.push_back(evm_common::xh256_t(new_owner.to_h256()));
            evm_common::xevm_log_t log{contract_address, topics};
            result[31] = 1;

            output.cost = transfer_ownership_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(std::move(log));
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = transfer_ownership_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transferOwnership reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    case method_id_set_controller: {
        xdbg("predefined erc20 contract: setController");

        uint64_t const set_controller_gas_cost = 3155;
        xbytes_t result(32, 0);

        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = set_controller_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: setController is not allowed in static context");

            return false;
        }

        if (erc20_token_id == common::xtoken_id_t::top) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

            xwarn("predefined erc20 contract: setController not supported token: %d", static_cast<int>(erc20_token_id));

            return false;
        }

        assert(erc20_token_id == common::xtoken_id_t::eth || erc20_token_id == common::xtoken_id_t::usdc || erc20_token_id == common::xtoken_id_t::usdt);

        // only contract owner can set controller.
        auto contract_state = state_ctx->load_unit_state(evm_erc20_contract_address.vaccount());
        auto const & msg_sender = common::xaccount_address_t::build_from(context.caller, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
        auto const & token_owner = contract_state->tep_token_owner(erc20_token_id);
        if (msg_sender != token_owner) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: setController called by non-admin account %s", context.caller.c_str());

            return false;
        }
        ec.clear();

        if (target_gas < set_controller_gas_cost) {
            err.fail_status = precompile_error::Error;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitError::OutOfGas);

            xwarn("predefined erc20 contract: setController out of gas, gas remained %" PRIu64 " gas required %" PRIu64, target_gas, set_controller_gas_cost);

            return false;
        }

        if (abi_decoder.size() != 1) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: setController with invalid parameter");

            return false;
        }

        auto const new_controller = abi_decoder.extract<common::xeth_address_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);

            xwarn("predefined erc20 contract: setController with invalid burn from address");

            return false;
        }

        if (new_controller.empty()) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = set_controller_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: transferOwnership reverted. new owner is zero");

            return false;
        }

        auto const & old_controller = common::xeth_address_t::build_from(contract_state->tep_token_controller(erc20_token_id));
        contract_state->tep_token_controller(erc20_token_id, common::xaccount_address_t::build_from(new_controller, base::enum_vaccount_addr_type_secp256k1_evm_user_account), ec);

        if (!ec) {
            auto const & contract_address = context.address;

            evm_common::xh256s_t topics;
            topics.push_back(evm_common::xh256_t(event_hex_string_controller_set));
            topics.push_back(evm_common::xh256_t(old_controller.to_h256()));
            topics.push_back(evm_common::xh256_t(new_controller.to_h256()));
            evm_common::xevm_log_t log{contract_address, topics};
            result[31] = 1;

            output.cost = set_controller_gas_cost;
            output.exit_status = Returned;
            output.output = result;
            output.logs.push_back(std::move(log));
        } else {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = set_controller_gas_cost;
            err.output = result;

            xwarn("predefined erc20 contract: setController reverted. ec %" PRIi32 " category %s msg %s", ec.value(), ec.category().name(), ec.message().c_str());
        }

        return !ec;
    }

    default: {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);

        xwarn("predefined erc20 contract: not supported method_id: %" PRIx32, function_selector.method_id);

        return false;
    }
    }
}

NS_END4
