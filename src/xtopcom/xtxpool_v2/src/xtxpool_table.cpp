// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_table.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xtable_bstate.h"
#include "xmbus/xevent_behind.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xvledger/xvledger.h"

namespace top {
namespace xtxpool_v2 {

#define state_update_too_long_time (600)

data::xtablestate_ptr_t xtxpool_table_t::get_target_tablestate(base::xvblock_t * block) const {
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block);
    if (bstate == nullptr) {
        return nullptr;
    }
    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());
    return tablestate;
}

bool xtxpool_table_t::get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) const {
    // TODO(jimmy) try sync behind account unit, make a new function
    auto latest_table = m_para->get_vblockstore()->get_latest_committed_block(m_xtable_info);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(committed_block.get());
    if (bstate == nullptr) {
        xwarn("xtxpool_table_t::get_account_basic_info fail-get tablestate. block=%s", committed_block->dump().c_str());
        return false;
    }

    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(bstate.get());
    base::xaccount_index_t account_index;
    tablestate->get_account_index(account, account_index);
    base::xvaccount_t _account_vaddress(account);
    base::xauto_ptr<base::xvblock_t> _block_ptr = m_para->get_vblockstore()->get_latest_cert_block(_account_vaddress);
    xblock_ptr_t latest_cert_unit = xblock_t::raw_vblock_to_object_ptr(_block_ptr.get());
    if (latest_cert_unit->get_height() < account_index.get_latest_unit_height()) {
        base::xauto_ptr<base::xvblock_t> _start_block_ptr = m_para->get_vblockstore()->get_latest_connected_block(_account_vaddress);
        if (account_index.get_latest_unit_height() > _start_block_ptr->get_height()) {
            account_index_info.set_sync_height_start(_start_block_ptr->get_height() + 1);
            account_index_info.set_sync_num(account_index.get_latest_unit_height() - _start_block_ptr->get_height());
        }
        return false;
    }

    base::xauto_ptr<base::xvblock_t> _start_block_ptr = m_para->get_vblockstore()->get_latest_committed_block(_account_vaddress);
    base::xauto_ptr<base::xvbstate_t> account_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_start_block_ptr.get());
    if (account_bstate == nullptr) {
        xwarn("xtxpool_table_t::get_account_basic_info fail-get unitstate. block=%s", _start_block_ptr->dump().c_str());
        return false;
    }

    xaccount_ptr_t account_state = std::make_shared<xunit_bstate_t>(account_bstate.get());
    // account_index_info.set_latest_block(latest_unit);
    account_index_info.set_latest_state(account_state);
    account_index_info.set_account_index(account_index);
    return true;
}

int32_t xtxpool_table_t::push_send_tx(const std::shared_ptr<xtx_entry> & tx) {
    int32_t ret = verify_send_tx(tx->get_tx());
    if (ret != xsuccess) {
        return ret;
    }

    // todo: flow contral by receipt id.
    // if (is_unconfirm_txs_reached_upper_limmit()) {
    //     xtxpool_warn("xtxpool_table_t::push_send_tx unconfirm txs reached upper limmit tx:%s", tx->get_tx()->dump().c_str());
    //     XMETRICS_COUNTER_INCREMENT("txpool_push_tx_send_fail_unconfirm_reached_limmit", 1);
    //     XMETRICS_COUNTER_INCREMENT("txpool_push_tx_send_fail", 1);
    //     return xtxpool_error_account_unconfirm_txs_reached_upper_limit;
    // }

    if (tx->get_tx()->is_send_tx()) {
        if (m_unconfirmed_tx_num >= table_unconfirm_txs_num_max) {
            xtxpool_warn("xtxpool_table_t::push_send_tx node unconfirm txs reached upper limmit tx:%s", tx->get_tx()->dump().c_str());
            return xtxpool_error_account_unconfirm_txs_reached_upper_limit;
        }

        base::xvaccount_t vaccount(tx->get_tx()->get_target_addr());
        auto peer_table_sid = vaccount.get_short_table_id();
        if (m_receipt_state_cache.is_unconfirmed_num_reach_limmit(peer_table_sid)) {
            xtxpool_warn("xtxpool_table_t::push_send_tx table-table unconfirm txs reached upper limmit tx:%s,peer_sid:%d", tx->get_tx()->dump().c_str(), peer_table_sid);
            return xtxpool_error_account_unconfirm_txs_reached_upper_limit;
        }
    }

    uint64_t latest_nonce;
    uint256_t latest_hash;
    bool result = get_account_latest_nonce_hash(tx->get_tx()->get_source_addr(), latest_nonce, latest_hash);
    if (!result) {
        // todo : push to non_ready_accounts
        // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
        // m_non_ready_accounts.push_tx(tx);
        xtxpool_warn("xtxpool_table_t::push_send_tx account state fall behind tx:%s", tx->get_tx()->dump(true).c_str());
        return xtxpool_error_account_state_fall_behind;
    }

    if (data::is_sys_contract_address(common::xaccount_address_t{tx->get_tx()->get_source_addr()})) {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_system);
    } else {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_normal);
    }

    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        if (m_locked_txs.try_push_tx(tx)) {
            return xsuccess;
        }
        ret = m_txmgr_table.push_send_tx(tx, latest_nonce, latest_hash);
    }
    if (ret != xsuccess) {
        XMETRICS_COUNTER_INCREMENT("txpool_push_tx_send_fail", 1);
    }
    return ret;
}

int32_t xtxpool_table_t::push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send) {
    uint64_t latest_receipt_id = m_receipt_state_cache.get_tx_corresponding_latest_receipt_id(tx);
    uint64_t tx_receipt_id = tx->get_tx()->get_last_action_receipt_id();
    if (tx_receipt_id < latest_receipt_id) {
        xtxpool_warn("xtxpool_table_t::push_receipt duplicate receipt:%s,id:%llu:%llu", tx->get_tx()->dump().c_str(), tx_receipt_id, latest_receipt_id);
        XMETRICS_COUNTER_INCREMENT("txpool_receipt_duplicate", 1);
        return xtxpool_error_tx_duplicate;
    }

    // check if receipt is repeat before verify it, because receipt verify is a time-consuming job.
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        bool is_repeat = m_txmgr_table.is_repeat_tx(tx);
        if (is_repeat) {
            xtxpool_warn("xtxpool_table_t::push_receipt repeat receipt:%s", tx->get_tx()->dump().c_str());
            XMETRICS_COUNTER_INCREMENT("txpool_receipt_repeat", 1);
            return xtxpool_error_request_tx_repeat;
        }
    }

    int32_t ret = xsuccess;
    if (!is_self_send) {
        int32_t ret = verify_receipt_tx(tx->get_tx());
        if (ret != xsuccess) {
            return ret;
        }
    }

    // auto & account_addr = tx->get_tx()->get_account_addr();

    // store::xaccount_basic_info_t account_basic_info;
    // bool result = m_table_indexstore->get_account_basic_info(account_addr, account_basic_info);
    // if (!result) {
    //     // todo : push to non_ready_accounts
    //     // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    //     // m_non_ready_accounts.push_tx(tx);
    //     xtxpool_warn("xtxpool_table_t::push_receipt account state fall behind tx:%s", tx->get_tx()->dump(true).c_str());
    //     return xtxpool_error_account_state_fall_behind;
    // }
    // auto latest_unit_block = m_para->get_vblockstore()->get_latest_committed_block(account_addr);

    // bool deny = false;
    // enum_xtxpool_error_type ret_r = reject(account_addr, tx->get_tx(), account_basic_info.get_latest_block()->get_height(), deny);
    // if (deny) {
    //     XMETRICS_COUNTER_INCREMENT("txpool_push_tx_receipt_fail", 1);
    //     xtxpool_warn("xtxpool_table_t::push_receipt reject tx:%s,ret:%u", tx->get_tx()->dump(true).c_str(), ret_r);
    //     return xtxpool_error_tx_duplicate;
    // }

    if (data::is_sys_contract_address(common::xaccount_address_t{tx->get_tx()->get_account_addr()})) {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_system);
    } else {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_normal);
    }

    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        ret = m_txmgr_table.push_receipt(tx);
    }
    if (ret != xsuccess) {
        XMETRICS_COUNTER_INCREMENT("txpool_push_tx_receipt_fail", 1);
    }
    return ret;
}

std::shared_ptr<xtx_entry> xtxpool_table_t::pop_tx(const tx_info_t & txinfo, bool clear_follower) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        bool exist = false;
        auto tx_ent = m_locked_txs.pop_tx(txinfo, exist);
        if (exist) {
            return tx_ent;
        }
        tx_ent = m_txmgr_table.pop_tx(txinfo, clear_follower);
        if (tx_ent != nullptr) {
            return tx_ent;
        }
    }

    // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    // return m_non_ready_accounts.pop_tx(txinfo);
    return nullptr;
}

void xtxpool_table_t::pop_tx(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        bool exist = false;
        if (txinfo.get_subtype() == enum_transaction_subtype_self || txinfo.get_subtype() == enum_transaction_subtype_send) {
            m_locked_txs.pop_tx(txinfo, exist);
        }

        m_txmgr_table.pop_tx(txinfo, peer_table_sid, receiptid);
        return;
    }

    // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    // m_non_ready_accounts.pop_tx(txinfo);
}

ready_accounts_t xtxpool_table_t::get_ready_accounts(const xtxs_pack_para_t & pack_para) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_ready_accounts(pack_para);
}

std::vector<xcons_transaction_ptr_t> xtxpool_table_t::get_ready_txs(const xtxs_pack_para_t & pack_para) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_ready_txs(pack_para);
}

const std::shared_ptr<xtx_entry> xtxpool_table_t::query_tx(const std::string & account, const uint256_t & hash) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        auto tx_ent = m_txmgr_table.query_tx(account, hash);
        if (tx_ent != nullptr) {
            return tx_ent;
        }
    }

    // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    // return m_non_ready_accounts.find_tx(account, hash);
    return nullptr;
}

void xtxpool_table_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce, const uint256_t & latest_hash) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.updata_latest_nonce(account_addr, latest_nonce, latest_hash);
}

bool xtxpool_table_t::is_account_need_update(const std::string & account_addr) const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.is_account_need_update(account_addr);
}

enum_xtxpool_error_type xtxpool_table_t::reject(const std::string & account_addr, const xcons_transaction_ptr_t & tx, uint64_t pre_unitblock_height, bool & deny) {
    enum_xtxpool_error_type reject_ret = xtxpool_success;
    // {
    //     std::lock_guard<std::mutex> lck(m_filter_mutex);
    //     reject_ret = m_table_filter.reject(account_addr, tx, pre_unitblock_height, deny);
    // }

    // if (reject_ret != xtxpool_success || deny) {
    //     xtxpool_info("xtxpool_table_t::reject reject tx:%s,ret:%u,deny:%d", tx->dump(true).c_str(), reject_ret, deny);
    // }
    return reject_ret;
}

// xcons_transaction_ptr_t xtxpool_table_t::get_unconfirm_tx(const std::string & account_addr, const uint256_t & hash) {
//     std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
//     return m_unconfirmed_tx_queue.find(account_addr, hash);
// xcons_transaction_ptr_t unconfirm_tx = nullptr;
// std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
// {
//     std::lock_guard<std::mutex> lck(m_filter_mutex);
//     unconfirm_tx = m_table_filter.get_tx(account_addr, hash_str);
// }

// if (unconfirm_tx == nullptr) {
//     auto latest_unit_block = m_para->get_vblockstore()->get_latest_committed_block(account_addr);
//     if (latest_unit_block != nullptr) {
//         std::lock_guard<std::mutex> lck(m_filter_mutex);
//         m_table_filter.update_reject_rule(account_addr, dynamic_cast<xblock_t *>(latest_unit_block.get()));
//         unconfirm_tx = m_table_filter.get_tx(account_addr, hash_str);
//     }
// }
// return unconfirm_tx;
// }

const std::vector<xcons_transaction_ptr_t> xtxpool_table_t::get_resend_txs(uint64_t now) {
    // todo:compatibility for old version without receipt id.
    // std::lock_guard<std::mutex> lck(m_filter_mutex);
    // return m_table_filter.get_resend_txs(now);
    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    return m_unconfirmed_tx_queue.get_resend_txs(now);
}

// bool xtxpool_table_t::is_unconfirm_txs_reached_upper_limmit() const {
//     std::lock_guard<std::mutex> lck(m_filter_mutex);
//     uint32_t num = m_table_filter.get_unconfirm_txs_num();
//     XMETRICS_COUNTER_SET("table_unconfirm_tx" + m_xtable_info.get_table_addr(), num);
//     xtxpool_warn("xtxpool_table_t::is_unconfirm_txs_reached_upper_limmit table:%s,num:%u,max:%u", m_xtable_info.get_table_addr().c_str(), num, table_unconfirm_txs_num_max);
//     return num >= table_unconfirm_txs_num_max;
// }

void xtxpool_table_t::unit_block_process(xblock_t * unit_block) {
    // // update_reject_rule(block->get_account(), block);
    // {
    //     std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    //     uint32_t tx_num_before = m_unconfirmed_tx_queue.size();
    //     m_unconfirmed_tx_queue.udpate_latest_confirmed_block(unit_block, m_receipt_state_cache);
    //     uint32_t tx_num_after = m_unconfirmed_tx_queue.size();
    //     if (tx_num_after > tx_num_before) {
    //         XMETRICS_COUNTER_INCREMENT("txpool_unconfirm_txs_num", tx_num_after - tx_num_before);
    //     } else if (tx_num_after < tx_num_before) {
    //         XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_txs_num", tx_num_before - tx_num_after);
    //     }
    // }

    if (unit_block->is_lightunit() && !unit_block->is_genesis_block()) {
        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(unit_block);
        const std::vector<xlightunit_tx_info_ptr_t> & txs = lightunit->get_txs();
        for (auto & tx : txs) {
            tx_info_t txinfo(unit_block->get_account(), tx->get_tx_hash_256(), tx->get_tx_subtype());
            pop_tx(txinfo, tx->get_receipt_id_tableid(), tx->get_receipt_id());
        }
    }

    // if (is_account_need_update(block->get_account())) {
    //     base::xauto_ptr<xblockchain2_t> blockchain(m_para->get_store()->clone_account(block->get_account()));
    //     xassert(blockchain != nullptr);
    //     updata_latest_nonce(block->get_account(), blockchain->account_send_trans_number(), blockchain->account_send_trans_hash());
    // }
}

void xtxpool_table_t::on_block_confirmed(xblock_t * table_block) {
    const auto & units = table_block->get_tableblock_units(false);
    if (!units.empty()) {
        for (auto & unit : units) {
            unit_block_process(unit.get());
        }
    }
}

int32_t xtxpool_table_t::verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs, uint64_t latest_commit_unit_height) {
    bool deny = false;
    for (auto it : txs) {
        int32_t ret = verify_cons_tx(it);
        if (ret != xsuccess) {
            xtxpool_warn("xtxpool_table_t::verify_txs verify fail,tx:%s,err:%u", it->dump(true).c_str(), ret);
            return ret;
        }
        // enum_xtxpool_error_type reject_ret = reject(account, it, latest_commit_unit_height, deny);
        // reject_ret = deny ? xtxpool_error_tx_duplicate : reject_ret;
        // if (reject_ret != xsuccess) {
        //     xtxpool_warn("xtxpool_table_t::verify_txs reject tx:%s,ret:%u,deny:%d", it->dump(true).c_str(), reject_ret, deny);
        //     return reject_ret;
        // }
    }
    return xsuccess;
}

void xtxpool_table_t::update_unconfirm_accounts() {
    // base::xauto_ptr<xblockchain2_t> blockchain{m_para->get_store()->clone_account(m_xtable_info.get_table_addr())};
    // if (blockchain != nullptr) {
    //     std::set<std::string> accounts = blockchain->get_unconfirmed_accounts();
    //     if (!accounts.empty()) {
    //         xtxpool_dbg("xtxpool_table_t::update_unconfirm_accounts unconfirmed accounts not empty size:%u", accounts.size());

    //         for (auto & account : accounts) {
    //             base::xauto_ptr<base::xvblock_t> unitblock = m_para->get_vblockstore()->get_latest_committed_block(account);
    //             if (unitblock != nullptr) {
    //                 xblock_t * block = dynamic_cast<xblock_t *>(unitblock.get());
    //                 update_reject_rule(account, block);
    //             }
    //         }
    //     }
    // }
    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    uint32_t tx_num_before = m_unconfirmed_tx_queue.size();
    m_unconfirmed_tx_queue.recover(m_receipt_state_cache);
    uint32_t tx_num_after = m_unconfirmed_tx_queue.size();
    m_unconfirmed_tx_num = m_unconfirmed_tx_queue.size();
    // XMETRICS_COUNTER_SET("table_unconfirm_txs_num" + m_xtable_info.get_table_addr(), m_unconfirmed_tx_num);
    if (tx_num_after > tx_num_before) {
        XMETRICS_COUNTER_INCREMENT("txpool_unconfirm_txs_num", tx_num_after - tx_num_before);
    } else if (tx_num_after < tx_num_before) {
        XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_txs_num", tx_num_before - tx_num_after);
    }
}

// void xtxpool_table_t::update_non_ready_accounts() {
// std::vector<std::shared_ptr<xtx_entry>> repush_txs;
// {
//     std::lock_guard<std::mutex> lck(m_non_ready_mutex);
//     auto account_addrs = m_non_ready_accounts.get_accounts();
//     for (auto & account_addr : account_addrs) {
//         store::xaccount_basic_info_t account_basic_info;
//         bool result = m_table_indexstore->get_account_basic_info(account_addr, account_basic_info);
//         if (result) {
//             auto tx_ents = m_non_ready_accounts.pop_account_txs(account_addr);
//             repush_txs.insert(repush_txs.end(), tx_ents.begin(), tx_ents.end());
//         }
//     }
// }

// for (auto &tx_ent : repush_txs) {
//     if (tx_ent->get_tx()->is_self_tx() || tx_ent->get_tx()->is_send_tx()) {
//         push_send_tx(tx_ent);
//     } else {
//         push_receipt(tx_ent);
//     }
// }
// }

void xtxpool_table_t::update_locked_txs(const std::vector<tx_info_t> & locked_tx_vec) {
    std::vector<std::shared_ptr<xtx_entry>> unlocked_txs;
    int32_t ret = xsuccess;
    locked_tx_map_t locked_tx_map;
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        for (auto & txinfo : locked_tx_vec) {
            // only send txs move to m_locked_txs, receipts need not move, because they canbe deduplicate by receipt id.
            if (txinfo.get_subtype() == enum_transaction_subtype_self || txinfo.get_subtype() == enum_transaction_subtype_send) {
                auto tx_ent = m_txmgr_table.pop_tx(txinfo,false);
                std::shared_ptr<locked_tx_t> locked_tx = std::make_shared<locked_tx_t>(txinfo, tx_ent);
                locked_tx_map[txinfo.get_hash_str()] = locked_tx;
            }
        }
        m_locked_txs.update(locked_tx_map, unlocked_txs);
    }

    // roll back txs push to txpool again. all locked txs are send tx
    for (auto & unlocked_tx : unlocked_txs) {
        uint64_t latest_nonce;
        uint256_t latest_hash;
        bool result = get_account_latest_nonce_hash(unlocked_tx->get_tx()->get_source_addr(), latest_nonce, latest_hash);
        if (!result) {
            // todo : push to non_ready_accounts
            xtxpool_warn("xtxpool_table_t::update_locked_txs account state fall behind tx:%s", unlocked_tx->get_tx()->dump().c_str());
            continue;
        }
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        ret = m_txmgr_table.push_send_tx(unlocked_tx, latest_nonce, latest_hash);
        xtxpool_info("xtxpool_table_t::update_locked_txs roll back to txmgr table tx:%s,ret:%d", unlocked_tx->get_tx()->dump().c_str(), ret);
    }
}

void xtxpool_table_t::update_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state) {
    m_receipt_state_cache.update(receiptid_state);

    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    m_txmgr_table.update_receiptid_state(receiptid_state);
}

xcons_transaction_ptr_t xtxpool_table_t::get_unconfirmed_tx(const std::string & to_table_addr, uint64_t receipt_id) const {
    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    return m_unconfirmed_tx_queue.get_unconfirmed_tx(to_table_addr, receipt_id);
}

bool xtxpool_table_t::need_sync_lacking_receipts() const {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    if (m_receipt_state_cache.last_update_time() + state_update_too_long_time < now) {
        xwarn("xtxpool_table_t::need_sync_lacking_receipts too long time not update receipt state, not sync lacking receipts. table:%s,update time:%llu,now:%llu,threshold:%d",
              m_xtable_info.get_table_addr().c_str(),
              m_receipt_state_cache.last_update_time(),
              now,
              state_update_too_long_time);
        return false;
    }
    return true;
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_table_t::get_lacking_recv_tx_ids(uint32_t max_num) const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_lacking_recv_tx_ids(max_num);
}

const std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> xtxpool_table_t::get_lacking_confirm_tx_hashs(uint32_t max_num) const {
    std::vector<xtxpool_table_lacking_receipt_ids_t> lacking_ids_vec;
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        lacking_ids_vec = m_txmgr_table.get_lacking_confirm_tx_ids(max_num);
    }

    std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> lacking_hashs;

    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    for (auto & table_lacking_ids : lacking_ids_vec) {
        auto peer_sid = table_lacking_ids.get_peer_sid();
        xtxpool_table_lacking_confirm_tx_hashs_t table_lacking_hashs(peer_sid);
        std::vector<uint256_t> hash_vec;
        auto & receipt_ids = table_lacking_ids.get_receipt_ids();
        for (auto & receipt_id : receipt_ids) {
            auto cons_tx = m_unconfirmed_tx_queue.get_unconfirmed_tx(peer_sid, receipt_id);
            if (cons_tx == nullptr) {
                xtxpool_warn("xtxpool_table_t::get_lacking_confirm_tx_hashs unconfirm tx(peersid:%d,rid:%llu) not found", peer_sid, receipt_id);
            } else {
                table_lacking_hashs.add_receipt_hash(cons_tx->get_transaction()->digest());
            }
        }
        if (!table_lacking_hashs.empty()) {
            lacking_hashs.push_back(table_lacking_hashs);
        }
    }
    return lacking_hashs;
}

// enum_xtxpool_error_type xtxpool_table_t::update_reject_rule(const std::string & account, const data::xblock_t * unit_block) {
//     std::lock_guard<std::mutex> lck(m_filter_mutex);
//     return m_table_filter.update_reject_rule(account, unit_block);
// }

int32_t xtxpool_table_t::verify_cons_tx(const xcons_transaction_ptr_t & tx) const {
    int32_t ret;
    if (tx->is_send_tx() || tx->is_self_tx()) {
        ret = verify_send_tx(tx);
    } else if (tx->is_recv_tx()) {
        ret = verify_receipt_tx(tx);
    } else if (tx->is_confirm_tx()) {
        ret = verify_receipt_tx(tx);
    } else {
        ret = xtxpool_error_tx_invalid_type;
    }
    return ret;
}

int32_t xtxpool_table_t::verify_send_tx(const xcons_transaction_ptr_t & tx) const {
    // 1. validation check
    int32_t ret = xverifier::xtx_verifier::verify_send_tx_validation(tx->get_transaction());
    if (ret) {
        return ret;
    }
    // 2. legal check, include hash/signature check and white/black check
    ret = xverifier::xtx_verifier::verify_send_tx_legitimacy(tx->get_transaction(), make_observer(m_para->get_store()));
    if (ret) {
        return ret;
    }
    // 3. tx duration expire check
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    ret = xverifier::xtx_verifier::verify_tx_duration_expiration(tx->get_transaction(), now);
    if (ret) {
        return ret;
    }
    return xsuccess;
}

int32_t xtxpool_table_t::verify_receipt_tx(const xcons_transaction_ptr_t & tx) const {
    XMETRICS_TIME_RECORD("txpool_message_unit_receipt_push_receipt_verify_receipt_tx");
    // only check digest here for process too long zec_workload contract transaction receipt
    // should recover length check at later version
    if (!tx->get_transaction()->digest_check()) {
        xtxpool_warn("xtxpool_table_t::verify_receipt_tx digest check fail, tx:%s", tx->dump(true).c_str());
        return xtxpool_error_receipt_invalid;
    }

    if (!tx->verify_cons_transaction()) {
        return xtxpool_error_receipt_invalid;
    }

    const base::xvqcert_t * prove_cert;
    std::string prove_account;
    if (!tx->get_receipt_prove_cert_and_account(prove_cert, prove_account)) {
        return xtxpool_error_tx_multi_sign_error;
    }

    base::enum_vcert_auth_result auth_result = m_para->get_certauth()->verify_muti_sign(prove_cert, prove_account);
    if (auth_result != base::enum_vcert_auth_result::enum_successful) {
        int32_t ret = xtxpool_error_tx_multi_sign_error;
        xtxpool_warn("xtxpool_table_t::verify_receipt_tx fail. account=%s,tx=%s,auth_result:%d,fail-%u", prove_account.c_str(), tx->dump(true).c_str(), auth_result, ret);
        return ret;
    }
    return xsuccess;
}

bool xtxpool_table_t::get_account_latest_nonce_hash(const std::string account_addr, uint64_t & latest_nonce, uint256_t & latest_hash) const {
    xaccount_basic_info_t account_basic_info;
    bool result = get_account_basic_info(account_addr, account_basic_info);
    if (!result) {
        if (account_basic_info.get_sync_num() > 0) {
            mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
                account_addr, account_basic_info.get_sync_height_start(), account_basic_info.get_sync_num(), true, "account_state_fall_behind");
            m_para->get_bus()->push_event(ev);
            xtxpool_info("xtxpool_table_t::get_account_latest_nonce_hash account:%s state fall behind,try sync unit from:%llu,count:%u",
                         account_addr.c_str(),
                         account_basic_info.get_sync_height_start(),
                         account_basic_info.get_sync_num());
        }
        return false;
    }

    latest_nonce = account_basic_info.get_latest_state()->account_send_trans_number();
    latest_hash = account_basic_info.get_latest_state()->account_send_trans_hash();
    return true;
}

}  // namespace xtxpool_v2
}  // namespace top
