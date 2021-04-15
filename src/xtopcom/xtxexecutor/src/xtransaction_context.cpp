// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include "xbase/xlog.h"

#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xconfig/xconfig_register.h"
#include "xverifier/xverifier_utl.h"

using namespace top::data;

NS_BEG2(top, txexecutor)

int32_t xtransaction_face_t::source_confirm_action_exec() {  // only for across account transaction
    return 0;
}

int32_t xtransaction_context_t::parse() {
    switch (m_trans->get_transaction()->get_tx_type()) {
#ifdef DEBUG  // debug use
        case xtransaction_type_create_user_account:
            m_trans_obj = std::make_shared<xtransaction_create_user_account>(m_account_ctx, m_trans);
            break;
#endif
        case xtransaction_type_create_contract_account:
            m_trans_obj = std::make_shared<xtransaction_create_contract_account>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_run_contract:
            m_trans_obj = std::make_shared<xtransaction_run_contract>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_transfer:
            m_trans_obj = std::make_shared<xtransaction_transfer>(m_account_ctx, m_trans);
            break;

        case xtransaction_type_set_account_keys:
            m_trans_obj = std::make_shared<xtransaction_set_keys>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_lock_token:
            m_trans_obj = std::make_shared<xtransaction_lock_token>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_unlock_token:
            m_trans_obj = std::make_shared<xtransaction_unlock_token>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_alias_name:
            m_trans_obj = std::make_shared<xtransaction_alias_name>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_create_sub_account:
            m_trans_obj = std::make_shared<xtransaction_create_sub_account>(m_account_ctx, m_trans);
            break;

        case xtransaction_type_vote:
            m_trans_obj = std::make_shared<xtransaction_vote>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_abolish_vote:
            m_trans_obj = std::make_shared<xtransaction_abolish_vote>(m_account_ctx, m_trans);
            break;

        case xtransaction_type_pledge_token_tgas:
            m_trans_obj = std::make_shared<xtransaction_pledge_token_tgas>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_redeem_token_tgas:
            m_trans_obj = std::make_shared<xtransaction_redeem_token_tgas>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_pledge_token_disk:
            m_trans_obj = std::make_shared<xtransaction_pledge_token_disk>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_redeem_token_disk:
            m_trans_obj = std::make_shared<xtransaction_redeem_token_disk>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_pledge_token_vote:
            m_trans_obj = std::make_shared<xtransaction_pledge_token_vote>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_redeem_token_vote:
            m_trans_obj = std::make_shared<xtransaction_redeem_token_vote>(m_account_ctx, m_trans);
            break;

        default:
            xwarn("invalid tx type:%d", m_trans->get_transaction()->get_tx_type());
            return xtransaction_parse_type_invalid;
    }

    try {
        auto ret = m_trans_obj->parse();
        if (ret != xsuccess) {
            xwarn("[global_trace][unit_service][parse][fail]%s, ret:0x%x error:%s",
                m_trans->get_digest_hex_str().c_str(), ret, chainbase::xmodule_error_to_str(ret).c_str());
            return ret;
        }
    }
    catch(const std::exception& e) {
        xwarn("[global_trace][unit_service][parse][fail]%s, error:xchain_error_action_parse_exception",
            m_trans->get_digest_hex_str().c_str());
        return xchain_error_action_parse_exception;
    }
    return xsuccess;
}

uint64_t xtransaction_face_t::parse_vote_info(const std::string& para){
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)para.data(), para.size());
    std::map<std::string, uint64_t> vote_infos;
    stream >> vote_infos;
    uint64_t vote_num{0};
    for(auto info: vote_infos){
        vote_num += info.second;
    }
    return vote_num;
}

int32_t xtransaction_context_t::check() {
    int32_t ret;
    if (m_trans_obj == nullptr) {
        ret = parse();
        if (ret != xsuccess) {
            return ret;
        }
    }

    ret = m_trans_obj->check();

    if (ret != xsuccess) {
        return ret;
    }
    return xsuccess;
}

int32_t xtransaction_context_t::source_action_exec(xtransaction_result_t & result) {
    xassert(m_trans->get_source_addr() == m_account_ctx->get_address());
    int32_t ret = m_trans_obj->source_service_fee_exec();
    if (ret != xsuccess) {
        return ret;
    }
    ret = m_trans_obj->source_fee_exec();
    if (ret != xsuccess) {
        return ret;
    }
    return m_trans_obj->source_action_exec();
}

int32_t xtransaction_context_t::target_action_exec(xtransaction_result_t & result) {
    xassert((m_trans->get_target_addr() == m_account_ctx->get_address()) || (m_trans->is_self_tx() && data::is_black_hole_address(common::xaccount_address_t{m_trans->get_target_addr()})));
    int32_t ret = m_trans_obj->target_fee_exec();
    if (ret != xsuccess) {
        return ret;
    }
    if (m_trans->is_self_tx() && data::is_black_hole_address(common::xaccount_address_t{m_trans->get_target_addr()})) {
        return 0;
    }
    return m_trans_obj->target_action_exec();
}

int32_t xtransaction_context_t::source_confirm_action_exec(xtransaction_result_t & result) {
    xassert(m_trans->get_source_addr() == m_account_ctx->get_address());
    int32_t ret = m_trans_obj->source_confirm_fee_exec();
    if (ret != xsuccess) {
        return ret;
    }
    return m_trans_obj->source_confirm_action_exec();
}

int32_t xtransaction_context_t::exec(xtransaction_result_t & result) {
    int32_t ret;
    if ((m_account_ctx->get_address() != m_trans->get_source_addr())
    && (m_account_ctx->get_address() != m_trans->get_target_addr())) {
        assert(0);
    }

    ret = check();
    if (ret) {
        xwarn("[global_trace][unit_service][tx consensus][exec check][fail]%s error:%s",
            m_trans->get_digest_hex_str().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
        return ret;
    }

    if (m_trans->is_self_tx() || m_trans->is_send_tx()) {
        ret = source_action_exec(result);
        if (ret) {
            xwarn("[global_trace][unit_service][tx consensus][tx exec source action][fail]%s action_name:%s error:%s",
                m_trans->get_digest_hex_str().c_str(), m_trans->get_source_action().get_action_name().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return ret;
        } else {
            xdbg("[global_trace][unit_service][tx consensus][tx exec source action][success]%s action_name:%s transaction type:%d",
                m_trans->get_digest_hex_str().c_str(), m_trans->get_source_action().get_action_name().c_str(), m_trans->get_tx_subtype());
        }
    }
    if (m_trans->is_self_tx() || m_trans->is_recv_tx()) {
        ret = target_action_exec(result);
        if (ret) {
            xwarn("[global_trace][unit_service][tx consensus][tx exec target action][fail]%s action_name:%s error:%s",
                m_trans->get_digest_hex_str().c_str(), m_trans->get_target_action().get_action_name().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return ret;
        } else {
            xdbg("[global_trace][unit_service][tx consensus][tx exec target action][success]%s action_name:%s transaction type:%d",
                m_trans->get_digest_hex_str().c_str(), m_trans->get_target_action().get_action_name().c_str(), m_trans->get_tx_subtype());
        }
    }
    if (m_trans->is_confirm_tx()) {
        ret = source_confirm_action_exec(result);
        if (ret) {
            xwarn("[global_trace][unit_service][tx consensus][tx exec source confirm action][fail]%s action_name:%s error:%s",
                m_trans->get_digest_hex_str().c_str(), m_trans->get_source_action().get_action_name().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return ret;
        } else {
            xdbg("[global_trace][unit_service][tx consensus][tx exec source confirm action][success]%s action_name:%s transaction type:%d",
                m_trans->get_digest_hex_str().c_str(), m_trans->get_source_action().get_action_name().c_str(), m_trans->get_tx_subtype());
        }
    }

    bool bret = m_account_ctx->get_transaction_result(result);
    assert(bret == true);

    return xsuccess;
}

std::string xtransaction_face_t::assemble_lock_token_param(const uint64_t amount, const uint32_t version) const {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << version; // version
    stream << amount; // amount
    stream << xaction_lock_account_token::UT_time; // unlock_type
    stream << static_cast<uint32_t>(1); // size
    // convert redeem_frozen_interval from timer_interval to second
    auto const redeem_frozen_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(unlock_gas_staked_delay_time);
    stream << std::to_string(redeem_frozen_interval * 10);
    return std::string((char*)stream.data(), stream.size());
}

int32_t xtransaction_face_t::source_service_fee_exec() {
    m_fee.update_service_fee();
    int32_t ret = m_account_ctx->top_token_transfer_out(0, 0, m_fee.get_service_fee());
    m_trans->set_current_beacon_service_fee(m_fee.get_service_fee());
    return ret;
}

int32_t xtransaction_face_t::target_fee_exec() {
    if (m_trans->is_self_tx()) {
        return m_fee.update_fee_recv_self();
    }
    m_fee.update_fee_recv();
    return xsuccess;
}

int32_t xtransaction_face_t::source_confirm_fee_exec() {
    return m_fee.update_fee_confirm();
}

int32_t xtransaction_create_contract_account::source_fee_exec() {
    int32_t ret{0};
    if (m_fee.need_use_tgas_disk(m_trans->get_source_addr(), m_trans->get_target_addr(), m_target_action.m_code)) {
        ret = m_fee.update_tgas_disk_sender(m_source_action.m_asset_out.m_amount, true);
    }
    return ret;
}

int32_t xtransaction_create_contract_account::check() {
    if (!m_source_action.is_top_token() || xverifier::xtx_verifier::verify_account_min_deposit(m_source_action.m_asset_out.m_amount)) {
        return xconsensus_service_error_min_deposit_error;
    }
    if (m_trans->get_source_action().get_action_type() != data::xaction_type_asset_out ||
            m_trans->get_target_action().get_action_type() != data::xaction_type_create_contract_account) {
        return xconsensus_service_error_action_not_valid;
    }
    // check address relationship
    if (!is_user_contract_address(common::xaccount_address_t{m_trans->get_target_addr()})) {
        return xconsensus_service_error_addr_type_error;
    }
    std::string source_addr{m_trans->get_source_addr()};
    auto authorization = m_trans->get_target_action().get_authorization();
    if (authorization.empty() || xverifier::UNCOMPRESSED_PUBKEY_LEN != authorization.size() ) {
        return xconsensus_service_error_sign_error;
    }
    uint16_t    ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    utl::xecpubkey_t pub_key{(uint8_t *)authorization.data()};
    if (m_trans->get_target_addr() != pub_key.to_address(source_addr, base::enum_vaccount_addr_type_custom_contract, ledger_id)) {
        return xconsensus_service_error_sign_error;
    }
    return 0;
}

int32_t xtransaction_create_contract_account::source_action_exec() {
    int32_t ret = m_account_ctx->top_token_transfer_out(m_source_action.m_asset_out.m_amount, 0);
    if (ret) {
        return ret;
    }

    m_account_ctx->set_lock_balance_change(m_source_action.m_asset_out.m_amount);
    ret = m_account_ctx->sub_contract_sub_account_check(m_trans->get_target_addr());
    if (ret) {
        return ret;
    }
    ret = m_account_ctx->set_contract_sub_account(m_trans->get_target_addr());
    if (ret) {
        return ret;
    }

    // check if create contract target action can be executed successfully
    std::shared_ptr<xaccount_context_t> _account_context = std::make_shared<xaccount_context_t>(m_trans->get_target_addr(), m_account_ctx->get_store());
    ret = _account_context->set_contract_parent_account(m_source_action.m_asset_out.m_amount, m_trans->get_source_addr());
    if (ret) {
        return ret;
    }

    // set random seed, or else it would be fail
    _account_context->set_context_para(m_account_ctx->get_timer_height(), m_account_ctx->get_random_seed(), 0, 0);
    xtransaction_ptr_t tx;
    xtransaction_t* raw_tx = m_trans->get_transaction();
    raw_tx->add_ref();
    tx.attach(raw_tx);
    xvm::xtransaction_trace_ptr trace = m_vm_service.deal_transaction(tx, _account_context.get());
    if(trace->m_errno != xvm::enum_xvm_error_code::ok) {
        return static_cast<int32_t>(trace->m_errno);
    } else {
        return ret;
    }
}

int32_t xtransaction_create_contract_account::target_action_exec() {
    int32_t ret = m_account_ctx->set_contract_parent_account(m_source_action.m_asset_out.m_amount, m_trans->get_source_addr());
    if (ret) {
        return ret;
    }
    m_account_ctx->set_tgas_limit(m_target_action.m_tgas_limit);
    xtransaction_ptr_t tx;
    xtransaction_t* raw_tx = m_trans->get_transaction();
    raw_tx->add_ref();
    tx.attach(raw_tx);
    xvm::xtransaction_trace_ptr trace = m_vm_service.deal_transaction(tx, m_account_ctx);

    if(m_fee.need_use_tgas_disk(m_trans->get_target_addr(), m_trans->get_target_addr(), m_target_action.m_code)){
        ret= m_fee.update_tgas_disk_after_sc_exec(trace);
        xdbg("[target_action_exec] gas: %u, disk: %u", trace->m_tgas_usage, trace->m_disk_usage);
    }
    if(trace->m_errno != xvm::enum_xvm_error_code::ok){
        return static_cast<int32_t>(trace->m_errno);
    } else {
        return ret;
    }
}

int32_t xtransaction_clickonce_create_contract_account::source_fee_exec() {
    m_fee.update_service_fee();
    int32_t ret = m_account_ctx->top_token_transfer_out(0, 0, m_fee.get_service_fee());
    m_trans->set_current_beacon_service_fee(m_fee.get_service_fee());
    return ret;
}

int32_t xtransaction_clickonce_create_contract_account::check() {
    if (!m_source_action.is_top_token() || xverifier::xtx_verifier::verify_account_min_deposit(m_source_action.m_asset_out.m_amount)) {
        return xconsensus_service_error_min_deposit_error;
    }
    if (m_trans->get_source_action().get_action_type() != data::xaction_type_asset_out ||
        m_trans->get_target_action().get_action_type() != data::xaction_type_tep) {
        return xconsensus_service_error_action_not_valid;
    }
    // check address relationship
    if (!is_user_contract_address(common::xaccount_address_t{m_trans->get_target_addr()})) {
        return xconsensus_service_error_addr_type_error;
    }
    std::string source_addr{m_trans->get_source_addr()};
    auto authorization = m_trans->get_target_action().get_authorization();
    if (authorization.empty() || xverifier::UNCOMPRESSED_PUBKEY_LEN != authorization.size()) {
        return xconsensus_service_error_sign_error;
    }
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    utl::xecpubkey_t pub_key{(uint8_t *)authorization.data()};
    if (m_trans->get_target_addr() != pub_key.to_address(source_addr, base::enum_vaccount_addr_type_custom_contract, ledger_id)) {
        return xconsensus_service_error_sign_error;
    }
    return 0;
}

int32_t xtransaction_clickonce_create_contract_account::source_action_exec() {
    int32_t ret = m_account_ctx->top_token_transfer_out(m_source_action.m_asset_out.m_amount, 0);
    if (ret) {
        return ret;
    }

    m_account_ctx->set_lock_balance_change(m_source_action.m_asset_out.m_amount);
    ret = m_account_ctx->sub_contract_sub_account_check(m_trans->get_target_addr());
    if (ret) {
        return ret;
    }
    ret = m_account_ctx->set_contract_sub_account(m_trans->get_target_addr());
    if (ret) {
        return ret;
    }

    // check if create contract target action can be executed successfully
    std::shared_ptr<xaccount_context_t> _account_context = std::make_shared<xaccount_context_t>(m_trans->get_target_addr(), m_account_ctx->get_store());
    ret = _account_context->set_contract_parent_account(m_source_action.m_asset_out.m_amount, m_trans->get_source_addr());
    if (ret) {
        return ret;
    }

    // set random seed, or else it would be fail
    _account_context->set_context_para(m_account_ctx->get_timer_height(), m_account_ctx->get_random_seed(), 0, 0);
    xtransaction_ptr_t tx;
    xtransaction_t* raw_tx = m_trans->get_transaction();
    raw_tx->add_ref();
    tx.attach(raw_tx);
    xvm::xtransaction_trace_ptr trace = m_vm_service.deal_transaction(tx, _account_context.get());
    if (trace->m_errno != xvm::enum_xvm_error_code::ok) {
        return static_cast<int32_t>(trace->m_errno);
    } else {
        return ret;
    }
}

int32_t xtransaction_clickonce_create_contract_account::target_action_exec() {
    int32_t ret = m_account_ctx->set_contract_parent_account(m_source_action.m_asset_out.m_amount, m_trans->get_source_addr());
    if (ret) {
        return ret;
    }
    m_account_ctx->set_tgas_limit(m_target_action.tgas_limit);
    xtransaction_ptr_t tx;
    xtransaction_t* raw_tx = m_trans->get_transaction();
    raw_tx->add_ref();
    tx.attach(raw_tx);
    xvm::xtransaction_trace_ptr trace = m_vm_service.deal_transaction(tx, m_account_ctx);

    if (m_fee.need_use_tgas_disk(m_trans->get_target_addr(), m_trans->get_target_addr(), m_target_action.code)) {
        ret= m_fee.update_tgas_disk_after_sc_exec(trace);
        xdbg("[target_action_exec] gas: %u, disk: %u", trace->m_tgas_usage, trace->m_disk_usage);
    }
    if (trace->m_errno != xvm::enum_xvm_error_code::ok) {
        return static_cast<int32_t>(trace->m_errno);
    } else {
        return ret;
    }
}

int32_t xtransaction_run_contract::source_fee_exec() {
    int32_t ret{0};
    if (m_fee.need_use_tgas_disk(m_trans->get_source_addr(), m_trans->get_target_addr(), m_target_action.m_function_name)) {
        ret = m_fee.update_tgas_disk_sender(m_source_action.m_asset_out.m_amount, true);
    }
    return ret;
}

int32_t xtransaction_run_contract::source_action_exec() {
    if (m_source_action.m_asset_out.m_amount != 0) {
        int32_t ret = m_account_ctx->top_token_transfer_out(m_source_action.m_asset_out.m_amount, 0);
        if (ret) {
            return ret;
        }
        m_account_ctx->set_lock_balance_change(m_source_action.m_asset_out.m_amount);
    }
    return 0;
}

int32_t xtransaction_run_contract::target_action_exec() {
    // auto transfer in to contract account
    if (m_source_action.m_asset_out.m_amount != 0) {
        auto ret = m_account_ctx->top_token_transfer_in(m_source_action.m_asset_out.m_amount);
        if (0 != ret) {
            return ret;
        }
    }
    m_account_ctx->set_tgas_limit(m_account_ctx->get_tgas_limit());

    m_account_ctx->set_source_pay_info(m_source_action);
    xtransaction_ptr_t tx;
    xtransaction_t* raw_tx = m_trans->get_transaction();
    raw_tx->add_ref();
    tx.attach(raw_tx);
    xvm::xtransaction_trace_ptr trace = m_node.deal_transaction(tx, m_account_ctx);

    int32_t ret{0};
    if(m_fee.need_use_tgas_disk(m_trans->get_target_addr(), m_trans->get_target_addr(), m_target_action.m_function_name)){
        ret= m_fee.update_tgas_disk_after_sc_exec(trace);
        xdbg("[target_action_exec] gas: %u, disk: %u, tx_hash: %s, source: %s, target: %s", trace->m_tgas_usage, trace->m_disk_usage,
              m_trans->get_digest_hex_str().c_str(), m_trans->get_source_addr().c_str(), m_trans->get_target_addr().c_str());
    } else {
        m_trans->set_current_send_tx_lock_tgas(m_trans->get_last_action_send_tx_lock_tgas());
        m_trans->set_current_recv_tx_use_send_tx_tgas(0);
        m_trans->set_current_used_deposit(m_trans->get_last_action_used_deposit());
        xdbg("tgas_disk tx hash: %s, lock_tgas: %u, use_send_tx_tgas: %u, used_deposit: %u",
              m_trans->get_digest_hex_str().c_str(), m_trans->get_last_action_send_tx_lock_tgas(), 0, m_trans->get_last_action_used_deposit());
    }
    if(trace->m_errno != xvm::enum_xvm_error_code::ok){
        return static_cast<uint32_t>(trace->m_errno);
    } else {
        return ret;
    }
}

int32_t xtransaction_transfer::source_fee_exec() {
    int32_t ret{0};
    auto transfer_amount = m_source_action.m_asset_out.m_amount;
    // no check transfer amount for genesis state
    if (!is_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() }) && transfer_amount) {
        ret = m_fee.update_tgas_disk_sender(transfer_amount, false);
    }
    return ret;
}

int32_t xtransaction_set_keys::source_fee_exec() {
    return m_fee.update_tgas_disk_sender(0, false);
}

int32_t xtransaction_lock_token::source_fee_exec() {
    return m_fee.update_tgas_disk_sender(m_target_action.m_amount, false);
}

int32_t xtransaction_unlock_token::source_fee_exec() {
    return m_fee.update_tgas_disk_sender(0, false);
}

int32_t xtransaction_create_sub_account::source_fee_exec() {
    return m_fee.update_tgas_disk_sender(m_source_action.m_asset_out.m_amount, false);
}

int32_t xtransaction_alias_name::source_fee_exec() {
    return m_fee.update_tgas_disk_sender(0, false);
}

// int32_t xtransaction_vote::source_fee_exec() {
//     return 0;
// }

// int32_t xtransaction_abolish_vote::source_fee_exec() {
//     return 0;
// }

int32_t xtransaction_pledge_token_vote::source_fee_exec(){
    m_lock_token = TOP_UNIT * m_target_action.m_vote_num / config::get_top_vote_rate(m_target_action.m_lock_duration);
    return m_fee.update_tgas_disk_sender(m_lock_token, false);
}

int32_t xtransaction_pledge_token_vote::source_action_exec() {
    return 0;
}

int32_t xtransaction_pledge_token_vote::target_action_exec() {
    auto ret = m_account_ctx->top_token_transfer_out(m_lock_token, 0);
    if(ret == 0){
        m_account_ctx->set_vote_balance_change(m_lock_token);
        m_account_ctx->set_unvote_num_change(m_target_action.m_vote_num);
        ret = m_account_ctx->update_pledge_vote_property(m_target_action);
    }
    return ret;
}

int32_t xtransaction_redeem_token_vote::source_fee_exec(){
    return m_fee.update_tgas_disk_sender(0, false);
}

int32_t xtransaction_redeem_token_vote::source_action_exec() {
    return 0;
}

int32_t xtransaction_redeem_token_vote::target_action_exec() {
    auto redeem_num = m_target_action.m_vote_num;
    if(redeem_num > m_account_ctx->get_blockchain()->unvote_num()){
        xdbg("pledge_redeem_vote, redeem_num: %llu, unvote_num: %llu", redeem_num, m_account_ctx->get_blockchain()->unvote_num());
        return xtransaction_pledge_redeem_vote_err;
    }
    m_account_ctx->merge_pledge_vote_property();
    return m_account_ctx->redeem_pledge_vote_property(redeem_num);
}

int32_t xtransaction_vote::source_fee_exec(){
    return m_fee.update_tgas_disk_sender(0, false);
}

int32_t xtransaction_vote::source_action_exec() {
    uint64_t vote_num = parse_vote_info(m_target_action.m_para);
    xdbg("pledge_redeem_token account: %s, vote_num: %llu, unvote_num: %llu",
          m_account_ctx->get_address().c_str(), vote_num, m_account_ctx->get_blockchain()->unvote_num());
    if(vote_num > m_account_ctx->get_blockchain()->unvote_num()) {
        return xtransaction_pledge_redeem_vote_err;
    }
    m_account_ctx->set_unvote_num_change(-vote_num);
    return 0;
}

int32_t xtransaction_vote::source_confirm_action_exec(){
    auto status = m_trans->get_last_action_exec_status();
    uint64_t vote_num = parse_vote_info(m_target_action.m_para);
    if(status == enum_xunit_tx_exec_status_fail){
        m_account_ctx->set_unvote_num_change(vote_num);
    }
    xdbg("pledge_redeem_vote account: %s, status: %d, vote_num: %llu", m_account_ctx->get_address().c_str(), status, vote_num);
    return 0;
}

int32_t xtransaction_abolish_vote::source_fee_exec() {
    return m_fee.update_tgas_disk_sender(0, false);
}

int32_t xtransaction_abolish_vote::source_action_exec() {
    return 0;
}

int32_t xtransaction_abolish_vote::source_confirm_action_exec() {
    auto status = m_trans->get_last_action_exec_status();
    xdbg("pledge_redeem_vote account: %s, status: %d", m_account_ctx->get_address().c_str(), status);
    if(status == enum_xunit_tx_exec_status_fail){
        return 0;
    }

    auto vote_num = parse_vote_info(m_target_action.m_para);
    xdbg("pledge_redeem_vote abolish account: %s, vote_num: %d, old unvote_num: %d",
          m_account_ctx->get_address().c_str(), vote_num, m_account_ctx->get_blockchain()->unvote_num());
    m_account_ctx->set_unvote_num_change(vote_num);
    return 0;
}

NS_END2
