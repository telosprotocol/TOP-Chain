﻿// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "xtxexecutor/xunit_service_error.h"

#include "xdata/xaction_parse.h"
#include "xdata/xdata_defines.h"
#include "xdata/xchain_param.h"
#include "xdata/xlightunit.h"
#include "xdata/xcons_transaction.h"
#include "xstore/xaccount_context.h"
#include "xvm/xvm_service.h"
#include "xbasic/xmodule_type.h"
#include "xbasic/xutility.h"
#include "xcrypto/xckey.h"
#include "xbase/xmem.h"
#include "xstore/xstore_error.h"
#include "xdata/xgenesis_data.h"
#include "xverifier/xtx_verifier.h"
#include "xtxexecutor/xtransaction_fee.h"

NS_BEG2(top, txexecutor)

using store::xaccount_context_t;
using data::xtransaction_result_t;
using data::xcons_transaction_ptr_t;

class xtransaction_face_t{
 public:
    xtransaction_face_t(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : m_account_ctx(account_ctx), m_trans(trans), m_fee(account_ctx, trans) {
    }
    virtual ~xtransaction_face_t() {}

    virtual int32_t parse() = 0;
    virtual int32_t check() {
        return xsuccess;
    }
    virtual int32_t source_action_exec() = 0;
    virtual int32_t target_action_exec() = 0;
    virtual int32_t source_confirm_action_exec();

    xtransaction_fee_t get_tx_fee() const {return m_fee;}
    virtual int32_t source_fee_exec() = 0;
    int32_t source_service_fee_exec();
    virtual int32_t target_fee_exec();
    virtual int32_t source_confirm_fee_exec();

    std::string assemble_lock_token_param(const uint64_t amount, const uint32_t version) const ;

    int32_t check_parent(){
        std::string parent;
        auto ret = m_account_ctx->get_parent_account(parent);
        if(ret != store::xaccount_property_parent_account_exist){
            return store::xaccount_property_parent_account_not_exist;
        }
        if(m_trans->get_source_addr() != parent){
            // source is not the owner of target
            return xtransaction_contract_pledge_token_source_target_mismatch;
        }
        return 0;
    }

    uint64_t parse_vote_info(const std::string& para);

 protected:
    xaccount_context_t*     m_account_ctx;
    xcons_transaction_ptr_t m_trans;
    xtransaction_fee_t      m_fee;
};

class xtransaction_create_user_account : public xtransaction_face_t{
 public:
    xtransaction_create_user_account(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }

    int32_t source_fee_exec() override {
#ifdef DEBUG
        return 0;
#else
        return m_fee.update_tgas_disk_sender(0, false);
#endif
    }

    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        return m_account_ctx->create_user_account(m_target_action.m_address);
    }
    int32_t target_fee_exec() override {
        return 0;
    };
    int32_t source_confirm_fee_exec() override {
        return 0;
    };
 private:
    data::xaction_source_null m_source_action;
    data::xaction_create_user_account m_target_action;
};

class xtransaction_create_contract_account : public xtransaction_face_t{
 public:
    xtransaction_create_contract_account(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }

    int32_t check() override ;
    int32_t source_fee_exec() override ;
    int32_t source_action_exec() override ;
    int32_t target_action_exec() override ;
    int32_t target_fee_exec() override {
        return 0;
    };
    int32_t source_confirm_fee_exec() override {
        if (m_trans->is_self_tx()) {
            return 0;
        }
        if (data::is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
            return 0;
        }
        return m_fee.update_contract_fee_confirm(m_source_action.m_asset_out.m_amount);
    };

 private:
    data::xaction_asset_out m_source_action;
    data::xaction_deploy_contract m_target_action;
    xvm::xvm_service m_vm_service;
};

class xtransaction_clickonce_create_contract_account : public xtransaction_face_t{
 public:
    xtransaction_clickonce_create_contract_account(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }

    int32_t check() override ;
    int32_t source_fee_exec() override ;
    int32_t source_action_exec() override ;
    int32_t target_action_exec() override ;
    int32_t target_fee_exec() override {
        return 0;
    };
    int32_t source_confirm_fee_exec() override {
        if (m_trans->is_self_tx()) {
            return 0;
        }
        if (data::is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
            return 0;
        }
        return m_fee.update_contract_fee_confirm(m_source_action.m_asset_out.m_amount);
    };

 private:
    data::xaction_asset_out m_source_action;
    data::xaction_deploy_clickonce_contract m_target_action;
    xvm::xvm_service m_vm_service;
};

class xtransaction_run_contract : public xtransaction_face_t{
 public:
    xtransaction_run_contract(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = 0;
        if (!m_trans->get_source_action().get_action_param().empty()) {
            ret = m_source_action.parse(m_trans->get_source_action());
        }
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        return 0;
    }

    int32_t source_fee_exec() override ;
    int32_t source_action_exec() override ;
    int32_t target_action_exec() override ;
    int32_t target_fee_exec() override {
        return 0;
    };
    int32_t source_confirm_fee_exec() override {
        if (m_trans->is_self_tx()) {
            return 0;
        }
        if (data::is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
            return 0;
        }
        return m_fee.update_contract_fee_confirm(m_source_action.m_asset_out.m_amount);
    };

 private:
    data::xaction_asset_out m_source_action;
    data::xaction_run_contract m_target_action;
    xvm::xvm_service m_node;
};

class xtransaction_transfer : public xtransaction_face_t{
 public:
    xtransaction_transfer(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {

         // src target address should different
        if (m_trans->get_source_addr() == m_trans->get_target_addr()) {
            return xverifier::xverifier_error::xverifier_error_src_dst_addr_same;
        }
        // tx min deposit
        if (!data::is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
            if (xverifier::xtx_verifier::verify_tx_min_deposit(m_trans->get_transaction()->get_deposit())) {
                return xverifier::xverifier_error::xverifier_error_tx_min_deposit_invalid;
            }
        }

        // min transfer amount
        if (m_source_action.m_asset_out.m_amount <= 0) {
             return xverifier::xverifier_error::xverifier_error_transfer_tx_min_amount_invalid;
        }

        if ( m_source_action.m_asset_out.m_amount > TOTAL_ISSUANCE){
            return xverifier::xverifier_error::xverifier_error_transfer_tx_amount_over_max;
        }

        // check source and dst amount
        if (m_source_action.m_asset_out.m_amount != m_target_action.m_asset.m_amount) {
            return xverifier::xverifier_error::xverifier_error_trnsafer_tx_src_dst_amount_not_same;
        }

        if (m_account_ctx != nullptr) {

            if(sys_contract_zec_reward_addr == m_account_ctx->get_address()) {
                return xverifier::xverifier_error::xverifier_success;
            }
        }


        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override {
        xdbg("tgas_disk xtransaction_transfer sender ");
        auto transfer_amount = m_source_action.m_asset_out.m_amount;
        int32_t ret = transfer_amount ? m_account_ctx->unlock_all_token() : 0;
        if(!ret){
            ret = m_account_ctx->token_transfer_out(m_source_action.m_asset_out, 0);
            if (!ret) {
                if (m_trans->get_target_addr() == black_hole_addr) {
                    m_trans->set_self_burn_balance(transfer_amount);  // TODO(jimmy) record burn balance in tx exec state
                }
            }
        }
        return ret;
    }

    int32_t target_action_exec() override {
        return m_account_ctx->token_transfer_in(m_target_action.m_asset);
    }

 private:
    data::xaction_asset_out m_source_action;
    data::xaction_asset_in m_target_action;
};

class xtransaction_pledge_token : public xtransaction_face_t{
 public:
    xtransaction_pledge_token(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        return 0;
    }

    virtual int32_t set_pledge_token_resource(uint64_t amount) = 0;

    int32_t source_fee_exec() override {
        if (!is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
            return m_fee.update_tgas_disk_sender(m_target_action.m_asset.m_amount, false);
        }
        return 0;
    }
    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        int32_t ret{0};
        if(m_trans->get_source_addr() != m_trans->get_target_addr()){
            ret = check_parent();
            if(ret != 0){
                return ret;
            }
        }
        ret = set_pledge_token_resource(m_target_action.m_asset.m_amount);
        if(ret != 0){
            return ret;
        }
        return m_account_ctx->top_token_transfer_out(m_target_action.m_asset.m_amount, 0);;
    }

private:
    data::xaction_source_null m_source_action;
    data::xaction_pledge_token m_target_action;
};

class xtransaction_redeem_token : public xtransaction_face_t{
 public:
    xtransaction_redeem_token(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        return 0;
    }

    virtual int32_t redeem_pledge_token_resource(uint64_t amount) = 0;
    virtual uint32_t redeem_type() = 0;

    int32_t source_fee_exec() override {
        int32_t ret{0};
        if (!is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
            ret = m_fee.update_tgas_disk_sender(0, false);
        }
        return ret;
    }
    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        int32_t ret{0};
        if(m_trans->get_source_addr() != m_trans->get_target_addr()){
            ret = check_parent();
            if(ret != 0){
                return ret;
            }
        }
        ret = redeem_pledge_token_resource(m_target_action.m_asset.m_amount);
        if(ret != 0){
            return ret;
        }
        m_account_ctx->token_transfer_in(m_target_action.m_asset);
        std::string param = assemble_lock_token_param(m_target_action.m_asset.m_amount, redeem_type());
        return m_account_ctx->lock_token(m_trans->get_transaction()->digest(), m_target_action.m_asset.m_amount, param);
    }

private:
    data::xaction_source_null m_source_action;
    data::xaction_redeem_token m_target_action;
};

class xtransaction_pledge_token_tgas : public xtransaction_pledge_token{
 public:
     xtransaction_pledge_token_tgas(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_pledge_token(account_ctx, trans) {
    }
    int32_t set_pledge_token_resource(uint64_t amount) override {
        return m_account_ctx->set_pledge_token_tgas(amount);
    }
};

class xtransaction_redeem_token_tgas : public xtransaction_redeem_token{
 public:
    xtransaction_redeem_token_tgas(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_redeem_token(account_ctx, trans) {
    }
    int32_t redeem_pledge_token_resource(uint64_t amount) override {
        return m_account_ctx->redeem_pledge_token_tgas(amount);
    }
    uint32_t redeem_type() { return 0; }
};

class xtransaction_pledge_token_disk : public xtransaction_pledge_token{
 public:
     xtransaction_pledge_token_disk(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_pledge_token(account_ctx, trans) {
    }
    int32_t set_pledge_token_resource(uint64_t amount) override {
        return m_account_ctx->set_pledge_token_disk(amount);
    }
};

class xtransaction_redeem_token_disk : public xtransaction_redeem_token{
 public:
    xtransaction_redeem_token_disk(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_redeem_token(account_ctx, trans) {
    }
    int32_t redeem_pledge_token_resource(uint64_t amount) override {
        return m_account_ctx->redeem_pledge_token_disk(amount);
    }
    uint32_t redeem_type() { return 1; }
};

class xtransaction_pledge_token_vote : public xtransaction_face_t{
 public:
    xtransaction_pledge_token_vote(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        uint16_t min_pledge_vote_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_stake_votes_num);
        // lock duration must be multiples of min_vote_lock_days and can not be 0
        if(m_target_action.m_vote_num < min_pledge_vote_num || m_target_action.m_lock_duration % config::min_vote_lock_days != 0 || m_target_action.m_lock_duration == 0){
            xdbg("pledge_token_vote err, vote_num: %u, duration: %u", m_target_action.m_vote_num, m_target_action.m_lock_duration);
            return xtransaction_pledge_redeem_vote_err;
        }
        return 0;
    }

    int32_t source_fee_exec() override ;
    int32_t source_action_exec() override ;
    int32_t target_action_exec() override ;

    uint64_t transform_vote_to_token(const data::xaction_pledge_token_vote& m_target_action);

private:
    data::xaction_source_null m_source_action;
    data::xaction_pledge_token_vote m_target_action;
    uint64_t m_lock_token{0};
};

class xtransaction_redeem_token_vote : public xtransaction_face_t{
 public:
    xtransaction_redeem_token_vote(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }
    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override ;
    int32_t target_action_exec() override ;

private:
    data::xaction_source_null m_source_action;
    data::xaction_redeem_token_vote m_target_action;
};

class xtransaction_set_keys : public xtransaction_face_t {
public:
    xtransaction_set_keys(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
        : xtransaction_face_t(account_ctx, trans)
    {}
    int32_t parse() override {
        return m_target_action.parse(m_trans->get_target_action());
    }
    int32_t check() override {
        if (m_target_action.m_account_key != data::XPROPERTY_ACCOUNT_VOTE_KEY
            && m_target_action.m_account_key != data::XPROPERTY_ACCOUNT_TRANSFER_KEY
            && m_target_action.m_account_key != data::XPROPERTY_ACCOUNT_DATA_KEY
            && m_target_action.m_account_key != data::XPROPERTY_ACCOUNT_CONSENSUS_KEY) {
            return -1;
        }
        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        return m_account_ctx->account_set_keys(m_target_action.m_account_key,
                                               m_target_action.m_key_value);
    }

private:
    //data::xaction_asset_out m_source_action;
    data::xaction_set_account_keys m_target_action;
};

class xtransaction_lock_token : public xtransaction_face_t {
public:
    xtransaction_lock_token(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
        : xtransaction_face_t(account_ctx, trans)
    {}
    int32_t parse() override {
        return m_target_action.parse(m_trans->get_target_action());
    }
    int32_t check() override {
        if (m_target_action.m_amount == 0) {
            return -1;
        }
        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        return m_account_ctx->lock_token(m_trans->get_transaction()->digest(),
                                         m_target_action.m_amount,
                                         m_target_action.m_params);
    }

private:
    data::xaction_lock_account_token m_target_action;    // TODO(jimmy) asset_in  maybe useless, default is asset in
};

class xtransaction_unlock_token : public xtransaction_face_t {
public:
    xtransaction_unlock_token(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
        : xtransaction_face_t(account_ctx, trans)
    {}
    int32_t parse() override {
        return m_target_action.parse(m_trans->get_target_action());
    }
    int32_t check() override {
        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        return m_account_ctx->unlock_token(m_trans->get_transaction()->digest(),
                                           m_target_action.m_lock_tran_hash,
                                           m_target_action.m_signatures);
    }

private:
    data::xaction_unlock_account_token m_target_action;    // TODO(jimmy) asset_in  maybe useless, default is asset in
};

class xtransaction_create_sub_account : public xtransaction_face_t{
 public:
    xtransaction_create_sub_account(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }

    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (!ret) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        if (!m_source_action.is_top_token() || !m_target_action.is_top_token()
            || xverifier::xtx_verifier::verify_account_min_deposit(m_source_action.m_asset_out.m_amount) || m_source_action.m_asset_out.m_amount != m_target_action.m_asset_out.m_amount) {
            return xconsensus_service_error_min_deposit_error;
        }

        if (m_trans->get_source_action().get_action_type() != data::xaction_type_asset_out ||
                m_trans->get_target_action().get_action_type() != data::xaction_type_asset_out) {
            return xconsensus_service_error_action_not_valid;
        }
        // sub_account sign check
        if (!is_sub_account_address(common::xaccount_address_t{ m_trans->get_target_addr() })) {
            return xconsensus_service_error_addr_type_error;
        }

        utl::xkeyaddress_t sub_account(m_trans->get_target_addr());
        utl::xecdsasig_t signature_obj((uint8_t *)m_trans->get_target_action().get_authorization().c_str());
        if (!sub_account.verify_signature(signature_obj, m_trans->get_target_action().sha2(), m_trans->get_source_addr())) {
            return xconsensus_service_error_sign_error;
        }
        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override {
        int32_t ret = m_account_ctx->sub_account_check(m_trans->get_target_addr());
        if (ret) {
            return ret;
        }
        ret = m_account_ctx->top_token_transfer_out(m_source_action.m_asset_out.m_amount, 0);
        if (!ret) {
            return m_account_ctx->set_sub_account(m_trans->get_target_addr());
        }
        return ret;
    }
    int32_t target_action_exec() override {
        return m_account_ctx->set_parent_account(m_target_action.m_asset_out.m_amount, m_trans->get_source_addr());
    }

 protected:
    data::xaction_asset_out m_source_action;
    data::xaction_asset_out m_target_action;
};

class xtransaction_alias_name : public xtransaction_face_t{
 public:
    xtransaction_alias_name(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : xtransaction_face_t(account_ctx, trans) {
    }

    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }
    int32_t check() override {
        auto const config_property_alias_name_max_len = XGET_CONFIG(config_property_alias_name_max_len);

        if (m_target_action.m_name.size() == 0
            || m_target_action.m_name.size() > config_property_alias_name_max_len) {
            return xconsensus_service_error_action_not_valid;
        }
        return 0;
    }

    int32_t source_fee_exec() override ;

    int32_t source_action_exec() override {
        return 0;
    }
    int32_t target_action_exec() override {
        return m_account_ctx->account_alias_name_set(m_target_action.m_name);
    }

 private:
    data::xaction_source_null m_source_action;
    data::xaction_alias_name m_target_action;
};

class xtransaction_context_t{
 public:
    xtransaction_context_t(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : m_account_ctx(account_ctx), m_trans(trans) {
        assert(trans != nullptr);
    }

    int32_t check();
    int32_t parse();
    int32_t exec(xtransaction_result_t & result);

 private:
    int32_t source_action_exec(xtransaction_result_t & result);
    int32_t target_action_exec(xtransaction_result_t & result);
    int32_t source_confirm_action_exec(xtransaction_result_t & result);

    std::shared_ptr<xtransaction_face_t> m_trans_obj{};
    xaccount_context_t* m_account_ctx;
    data::xcons_transaction_ptr_t m_trans;
};

class xtransaction_vote : public xtransaction_face_t {
public:
    xtransaction_vote(xaccount_context_t* account_ctx, const data::xcons_transaction_ptr_t& trans)
        : xtransaction_face_t(account_ctx, trans) {}

    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }

    int32_t check() override {
        return 0;
    }

    int32_t source_fee_exec() override ;
    int32_t source_confirm_action_exec() override ;
    int32_t source_action_exec() override ;

    int32_t target_action_exec() override {
        xtransaction_ptr_t tx;
        xtransaction_t* raw_tx = m_trans->get_transaction();
        raw_tx->add_ref();
        tx.attach(raw_tx);
        xvm::xtransaction_trace_ptr trace = m_node.deal_transaction(tx, m_account_ctx);
        return static_cast<uint32_t>(trace->m_errno);
    }

private:
    data::xaction_source_null m_source_action;
    data::xaction_run_contract m_target_action;
    xvm::xvm_service m_node;
};

class xtransaction_abolish_vote : public xtransaction_face_t {
public:
    xtransaction_abolish_vote(xaccount_context_t* account_ctx, const data::xcons_transaction_ptr_t& trans)
        : xtransaction_face_t(account_ctx, trans) {}

    int32_t parse() override {
        int32_t ret = m_source_action.parse(m_trans->get_source_action());
        if (ret == xsuccess) {
            ret = m_target_action.parse(m_trans->get_target_action());
        }
        return ret;
    }

    int32_t check() override {
        return 0;
    }

    int32_t source_fee_exec() override ;
    int32_t source_confirm_action_exec() override ;
    int32_t source_action_exec() override ;

    int32_t target_action_exec() override {
        xtransaction_ptr_t tx;
        xtransaction_t* raw_tx = m_trans->get_transaction();
        raw_tx->add_ref();
        tx.attach(raw_tx);
        xvm::xtransaction_trace_ptr trace = m_node.deal_transaction(tx, m_account_ctx);
        return static_cast<uint32_t>(trace->m_errno);
    }

private:
    data::xaction_source_null m_source_action;
    data::xaction_run_contract m_target_action;
    xvm::xvm_service m_node;
};

NS_END2
