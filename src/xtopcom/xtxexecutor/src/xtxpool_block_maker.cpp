﻿// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xtxexecutor/xtxpool_block_maker.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xblock_maker_para.h"
#include "xstore/xtgas_singleton.h"
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xdatautil.h"
#include "xstake/xstake_algorithm.h"
#include "xmbus/xevent_behind.h"
#include <cinttypes>
#include "xconfig/xconfig_register.h"
NS_BEG2(top, txexecutor)

xtable_blockmaker_t::xtable_blockmaker_t(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool) {
// m_store(store), m_blockstore(blockstore), m_txpool(txpool)
    m_resources = std::make_shared<xblockmaker_resources_impl_t>(store, make_observer(blockstore), make_observer(txpool));
}

xtable_blockmaker_t::~xtable_blockmaker_t() {

}

std::shared_ptr<xproposal_maker_face> xtable_blockmaker_t::get_proposal_maker(const std::string & account) {
    // TODO(jimmy)
    std::shared_ptr<xproposal_maker_face> proposal_maker = std::make_shared<xproposal_maker_t>(account, m_resources);
    return proposal_maker;
}

// enum xtxpool_blockmaker_para_t {
//     enum_check_tableblock_connection_max_num = 6,
//     enum_full_table_interval_max_num = 16,  // TODO(jimmy)
//     enum_empty_block_max_num = 2,
// };

// xtable_blockmaker_t::xtable_blockmaker_t(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool)
// : m_store(store), m_blockstore(blockstore), m_txpool(txpool) {
//     m_unit_blockmaker = new xunit_blockmaker_t(store, blockstore, txpool);
//     m_blockstore->add_ref();
// }

// xtable_blockmaker_t::~xtable_blockmaker_t() {
//     delete m_unit_blockmaker;
//     m_blockstore->release_ref();
// }

std::shared_ptr<xunit_service::xblock_maker_face> xblockmaker_factory::create_txpool_block_maker(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool) {
    auto blockmaker = std::make_shared<xtable_blockmaker_t>(store, blockstore, txpool);
    return blockmaker;
}

// base::xauto_ptr<base::xvblock_t> xtable_blockmaker_t::get_latest_block(const std::string &account) {
//     return m_blockstore->get_latest_cert_block(account);
// }

// xtxpool::xtxpool_table_face_t*  xtable_blockmaker_t::get_txpool_table(const std::string & table_account) {
//     return m_txpool->get_txpool_table(table_account);
// }

// void xtable_blockmaker_t::invoke_sync(const std::string & account, const std::string & reason) {
//     mbus::xevent_ptr_t block_event = std::make_shared<mbus::xevent_behind_origin_t>(account, mbus::enum_behind_type_common, reason);
//     m_store->get_mbus()->push_event(block_event);
// }

// int xtable_blockmaker_t::check_cert_connect(base::xvblock_t* latest_cert_block) {
//     if (latest_cert_block->is_genesis_block() || latest_cert_block->get_height() == 1) {
//         return xconsensus::enum_xconsensus_code_successful;
//     }

//     const std::string & account = latest_cert_block->get_account();
//     if (latest_cert_block->check_block_flag(base::enum_xvblock_flag_committed) || latest_cert_block->check_block_flag(base::enum_xvblock_flag_locked)) {
//         // latest_cert_block already behind, should retry
//         xwarn("xtable_blockmaker_t::check_cert_connect fail-cert block is commited or locked. %s", latest_cert_block->dump().c_str());
//         return xconsensus::enum_xconsensus_error_bad_height;
//     }

//     uint64_t cur_height = latest_cert_block->get_height() - 1;
//     uint64_t count = 0;
//     while (1) {
//         base::xauto_ptr<base::xvblock_t> cur_block = m_blockstore->load_block_object(account, cur_height, false);
//         if (cur_block == nullptr) {
//             xwarn("xtable_blockmaker_t::check_cert_connect,fail-missing block. latest_cert_block:%s cur_height:%ld",
//                 latest_cert_block->dump().c_str(), cur_height);
//             break;
//         }

//         const int block_flags = cur_block->get_block_flags();
//         if ((block_flags & base::enum_xvblock_flag_committed) != 0) {
//             // TODO(jimmy) now should has all tableblocks
//             if (xblocktool_t::is_connect_and_executed_block(cur_block.get())) {
//                 return xsuccess;
//             } else {
//                 xwarn("xtable_blockmaker_t::check_cert_connect,fail-commit but not connect execute block. latest_cert_block:%s invoke_block:%s",
//                     latest_cert_block->dump().c_str(), cur_block->dump().c_str());
//                 break;
//             }
//         }

//         count++;
//         if (count > enum_check_tableblock_connection_max_num) {
//             xwarn("xtable_blockmaker_t::check_cert_connect,fail-not find connect block. latest_cert_block:%s cur_height:%ld",
//                 latest_cert_block->dump().c_str(), cur_height);
//             break;
//         }
//         cur_height--;
//     }

//     return enum_xtxexecutor_error_leader_tableblock_behind;
// }

// int xtable_blockmaker_t::try_verify_tableblock(base::xvblock_t *proposal_block, base::xvblock_t* latest_cert_block, const xblock_consensus_para_t &cs_para, const xvip2_t& xip,
//                                                xtxpool::xtxpool_table_face_t* txpool_table, uint64_t committed_height) {
//     xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(proposal_block);
//     xtableblock_proposal_input_t table_proposal_input;
//     int32_t serialize_ret = table_proposal_input.serialize_from_string(proposal_block->get_input()->get_proposal());
//     if (serialize_ret <= 0) {
//         xerror("xtable_blockmaker_t::verify_block fail-table serialize from proposal input. proposal:%s at-node:%s",
//             proposal_block->dump().c_str(), data::xdatautil::xip_to_hex(xip).c_str());
//         return xconsensus::enum_xconsensus_error_bad_proposal;
//     }
//     const std::vector<xunit_proposal_input_t> & unit_proposal_inputs = table_proposal_input.get_unit_inputs();
//     if (unit_proposal_inputs.size() == 0) {
//         xerror("xtable_blockmaker_t::verify_block fail-table no proposal input. proposal:%s at-node:%s",
//             proposal_block->dump().c_str(), data::xdatautil::xip_to_hex(xip).c_str());
//         return xconsensus::enum_xconsensus_error_bad_proposal;
//     }

//     xtable_block_para_t table_para;
//     for (auto & input_unit : unit_proposal_inputs) {
//         base::xvblock_t* unit = nullptr;
//         int32_t ret = m_unit_blockmaker->verify_block(input_unit, cs_para, unit, txpool_table, committed_height);
//         if (ret) {
//             return ret;
//         }

//         base::xauto_ptr<base::xvblock_t> auto_release_unit(unit);  // TODO(jimmy) 后续统一改成智能指针
//         table_para.add_unit(unit);
//         unit->get_cert()->set_parent_height(proposal_block->get_height());
//         data::xblock_t* block = (data::xblock_t*)unit;
//         xdbg("xtable_blockmaker_t::verify_block unit info. %s unit:%s",
//             proposal_block->dump().c_str(),
//             block->dump().c_str());
//     }

//     table_para.set_extra_data(cs_para.get_extra_data());
//     base::xauto_ptr<data::xblock_t> local_tableblock = (data::xblock_t*)xtable_block_t::create_next_tableblock(table_para, latest_cert_block);
//     // base::xauto_ptr<base::xvblock_t> local_tableblock = verify_tableblock(proposal_block, units, latest_cert_block);
//     if (local_tableblock == nullptr) {
//         xerror("xtable_blockmaker_t::verify_block fail-make tableblock. proposal:%s at-node:%s",
//             proposal_block->dump().c_str(), data::xdatautil::xip_to_hex(xip).c_str());
//         return xconsensus::enum_xconsensus_error_bad_proposal;
//     }
//     local_tableblock->set_consensus_para(cs_para);
//     local_tableblock->get_cert()->set_nonce(proposal_block->get_cert()->get_nonce());
//     local_tableblock->get_cert()->set_validator(proposal_block->get_cert()->get_validator());
//     local_tableblock->get_cert()->set_auditor(proposal_block->get_cert()->get_auditor());
//     local_tableblock->get_cert()->set_justify_cert_hash(proposal_block->get_cert()->get_justify_cert_hash());

//     int ret = verify_out_tableblock(proposal_block, local_tableblock.get());
//     if (ret) {
//         return ret;
//     }
//     return xsuccess;
// }

// int xtable_blockmaker_t::verify_full_tableblock(base::xvblock_t *proposal_block,
//                                                 base::xvblock_t* latest_cert_block,
//                                                 base::xvblock_t* latest_commit_block,
//                                                 const std::vector<xblock_ptr_t> uncommit_blocks,
//                                                 const xblock_consensus_para_t &cs_para) {
//     xaccount_ptr_t commit_account = m_store->query_account(proposal_block->get_account());
//     if (commit_account == nullptr || commit_account->get_last_height() != latest_commit_block->get_height()) {
//         xerror("xtable_blockmaker_t::verify_block fail-backup account behind. proposal=%s",
//             proposal_block->dump().c_str());
//         return enum_xtxexecutor_error_account_hehind_block;
//     }

//     xtable_mbt_ptr_t last_full_table_mbt = make_object_ptr<xtable_mbt_t>();
//     if (latest_cert_block->get_last_full_block_height() != 0) {
//         std::string full_offstate_value = m_store->get_full_offstate(latest_cert_block->get_account(), latest_cert_block->get_last_full_block_height());
//         if (full_offstate_value.empty()) {
//             xerror("xtable_blockmaker_t::verify_block full offstate load fail. account=%s, full_height=%ld",
//                 latest_cert_block->get_account().c_str(), latest_cert_block->get_last_full_block_height());
//             return enum_xtxexecutor_error_account_hehind_block;
//         }
//         int32_t ret = last_full_table_mbt->serialize_from_string(full_offstate_value);
//         xassert(ret > 0);
//     }

//     base::xauto_ptr<data::xblock_t> local_tableblock = (data::xblock_t*)create_full_table(commit_account, latest_cert_block, uncommit_blocks, last_full_table_mbt, cs_para);
//     local_tableblock->get_cert()->set_nonce(proposal_block->get_cert()->get_nonce());
//     local_tableblock->get_cert()->set_validator(proposal_block->get_cert()->get_validator());
//     local_tableblock->get_cert()->set_auditor(proposal_block->get_cert()->get_auditor());
//     local_tableblock->get_cert()->set_justify_cert_hash(proposal_block->get_cert()->get_justify_cert_hash());

//     int ret = verify_out_tableblock(proposal_block, local_tableblock.get());
//     if (ret) {
//         return ret;
//     }
//     return xsuccess;
// }

// base::xvblock_t * xtable_blockmaker_t::make_block(const std::string &account, const xblock_maker_para_t & para, const xvip2_t& leader_xip) {
//     uint64_t committed_height = para.latest_blocks->get_latest_committed_block()->get_height();
//     base::xvblock_t* latest_cert_block = para.latest_blocks->get_latest_cert_block();
//     int ret;
//     base::xvblock_t* proposal_block = nullptr;
//     xblock_consensus_para_t cs_para;

//     bool is_empty_block_para = para.unit_accounts.size() == 0;
//     if (false == leader_set_consensus_para(latest_cert_block, para, cs_para, is_empty_block_para)) {
//         xwarn("xtable_blockmaker_t::make_block account=%s,viewid=%" PRIu64 ", fail-leader_set_consensus_para. latest_cert_block:%s at-node:%s",
//             account.c_str(), para.viewid, latest_cert_block->dump().c_str(), data::xdatautil::xip_to_hex(leader_xip).c_str());
//         return nullptr;
//     }

//     // firstly, try to make full table block
//     if (xblocktool_t::can_make_next_full_table(latest_cert_block, enum_full_table_interval_max_num)) {
//         ret = make_full_table(*para.latest_blocks, cs_para, proposal_block);
//         if (ret != xsuccess) {
//             xwarn("xtable_blockmaker_t::make_block account=%s,viewid=%" PRIu64 ", fail-leader_make_full_table. latest_cert_block:%s at-node:%s",
//                 account.c_str(), para.viewid, latest_cert_block->dump().c_str(), data::xdatautil::xip_to_hex(leader_xip).c_str());
//             return nullptr;
//         }
//     }
//     // secondly, try to make light table block
//     if (nullptr == proposal_block && para.unit_accounts.size() != 0) {
//         ret = make_tableblock(para.unit_accounts, latest_cert_block, cs_para, committed_height, para.txpool_table, proposal_block);
//         if (ret != xsuccess && !para.can_make_empty_block) {
//             xwarn("xtable_blockmaker_t::make_block account=%s,viewid=%" PRIu64 ", fail-not create tableblock, error:%s latest_cert_block:%s at-node:%s",
//                 account.c_str(), para.viewid, chainbase::xmodule_error_to_str(ret).c_str(), latest_cert_block->dump().c_str(), data::xdatautil::xip_to_hex(leader_xip).c_str());
//             return nullptr;
//         }
//     }
//     // thirdly, try to make empty table block
//     if (nullptr == proposal_block && para.can_make_empty_block) {
//         proposal_block = data::xblocktool_t::create_next_emptyblock(latest_cert_block);
//         data::xblock_t* block = (data::xblock_t*)proposal_block;
//         block->set_consensus_para(cs_para);
//     }
//     // finally, set consensus para for proposal block
//     if (nullptr == proposal_block) {
//         xerror("xtable_blockmaker_t::make_block account=%s,viewid=%" PRIu64 ", fail-not create any tableblock, latest_cert_block:%s at-node:%s",
//             account.c_str(), para.viewid, latest_cert_block->dump().c_str(), data::xdatautil::xip_to_hex(leader_xip).c_str());
//         return nullptr;
//     }

//     bool bret = proposal_block->reset_prev_block(latest_cert_block);
//     xassert(bret);
//     xkinfo("xtable_blockmaker_t::make_block account=%s,viewid=%" PRIu64 ", succ-create block=%s class=%d at-node:%s",
//         account.c_str(), para.viewid, proposal_block->dump().c_str(), proposal_block->get_block_class(), data::xdatautil::xip_to_hex(leader_xip).c_str());
//     return proposal_block;
// }

// int xtable_blockmaker_t::verify_block_class(base::xvblock_t *proposal_block, const base::xblock_mptrs & latest_blocks, base::xvblock_t* latest_cert_block) {
//     if (proposal_block->get_block_class() != base::enum_xvblock_class_light
//         && proposal_block->get_block_class() != base::enum_xvblock_class_nil
//         && proposal_block->get_block_class() != base::enum_xvblock_class_full) {
//         xerror("xtable_blockmaker_t::verify_block fail-block class invalid. %s", proposal_block->dump().c_str());
//         return enum_xtxexecutor_error_backup_verify_fail_block_class;
//     }

//     // height 1 should be light tableblock
//     if (proposal_block->get_height() == 1) {
//         if (proposal_block->get_block_class() != base::enum_xvblock_class_light) {
//             xerror("xtable_blockmaker_t::verify_block fail-height 1 should be light table. %s", proposal_block->dump().c_str());
//             return enum_xtxexecutor_error_backup_verify_fail_block_class;
//         }
//         return xsuccess;
//     }

//     // should always make full table if the condition is met
//     if (xblocktool_t::can_make_next_full_table(latest_cert_block, enum_full_table_interval_max_num)) {
//         if (proposal_block->get_block_class() != base::enum_xvblock_class_full) {
//             xerror("xtable_blockmaker_t::verify_block_class fail-should be full tableblock. proposal=%s", proposal_block->dump().c_str());
//             return enum_xtxexecutor_error_backup_verify_fail_block_class;
//         }
//         return xsuccess;
//     }

//     // should not make too many empty table
//     if (proposal_block->get_block_class() == base::enum_xvblock_class_nil) {
//         if (!xblocktool_t::can_make_next_empty_block(latest_blocks, enum_empty_block_max_num)) {
//             xerror("xtable_blockmaker_t::verify_block_class fail-should not be empty block. proposal=%s", proposal_block->dump().c_str());
//             return enum_xtxexecutor_error_backup_verify_fail_block_class;
//         }
//         return xsuccess;
//     }
//     return xsuccess;
// }

// int xtable_blockmaker_t::verify_block(base::xvblock_t *proposal_block, base::xvqcert_t * bind_clock_cert, base::xvqcert_t * bind_drand_cert,
//                                       xtxpool::xtxpool_table_face_t* txpool_table, uint64_t committed_height, const xvip2_t& xip) {
//     const auto & account = proposal_block->get_account();
//     base::xblock_mptrs latest_blocks = m_blockstore->get_latest_blocks(account);

//     std::vector<xblock_ptr_t> uncommit_blocks;
//     xblock_ptr_t latest_cert_block = nullptr;
//     int ret = check_proposal_connect(latest_blocks, proposal_block, uncommit_blocks, latest_cert_block);
//     if (ret) {
//         invoke_sync(account, "tableblock backup sync");
//         XMETRICS_COUNTER_INCREMENT("txexecutor_cons_invoke_sync", 1);
//         return ret;
//     }

//     ret = verify_block_class(proposal_block, latest_blocks, latest_cert_block.get());
//     if (ret) {
//         return ret;
//     }

//     xblock_consensus_para_t cs_para;
//     if (false == backup_set_consensus_para(latest_cert_block.get(), proposal_block, bind_drand_cert, cs_para)) {
//         return enum_xtxexecutor_error_backup_verify_fail_check_consensus_para;
//     }
//     if (proposal_block->get_block_class() == base::enum_xvblock_class_light) {
//         int ret = try_verify_tableblock(proposal_block, latest_cert_block.get(), cs_para, xip, txpool_table, committed_height);
//         if (ret) {
//             return ret;
//         }
//     }

//     if (proposal_block->get_block_class() == base::enum_xvblock_class_full) {
//         int ret = verify_full_tableblock(proposal_block, latest_cert_block.get(), latest_blocks.get_latest_committed_block(), uncommit_blocks, cs_para);
//         if (ret) {
//             return ret;
//         }
//     }

//     xkinfo("xtable_blockmaker_t::verify_block succ, proposal=%s at-node:%s",
//         proposal_block->dump().c_str(), data::xdatautil::xip_to_hex(xip).c_str());
//     return xconsensus::enum_xconsensus_code_successful;
// }

// int xtable_blockmaker_t::check_proposal_connect(const base::xblock_mptrs & latest_blocks,
//                                                 base::xvblock_t *proposal_block,
//                                                 std::vector<xblock_ptr_t> & uncommit_blocks,
//                                                 xblock_ptr_t & latest_cert_block) {
//     if (!xblocktool_t::verify_latest_blocks(latest_blocks)) {
//         xwarn("xtable_blockmaker_t::verify_block fail-verify latest blocks. proposal=%s", proposal_block->dump().c_str());
//         return enum_xtxexecutor_error_backup_tableblock_behind;
//     }

//     // for backup, proposal block may come from any block of last height, but it will probably come from default latest cert block
//     latest_cert_block = nullptr;
//     if (proposal_block->get_last_block_hash() == latest_blocks.get_latest_cert_block()->get_block_hash()) {
//         latest_cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
//     } else {
//         const std::string & account = proposal_block->get_account();
//         base::xvaccount_t _vaccount(account);
//         base::xblock_vector latest_certs = m_blockstore->query_block(_vaccount, proposal_block->get_height() - 1);
//         if (false == latest_certs.get_vector().empty()) {
//             for (auto & latest_cert : latest_certs.get_vector()) {
//                 if (latest_cert->get_block_hash() == proposal_block->get_last_block_hash()) {  // found conected prev one
//                     latest_cert_block = xblock_t::raw_vblock_to_object_ptr(latest_cert);
//                     break;
//                 }
//             }
//         }
//     }

//     if (latest_cert_block == nullptr) {
//         xwarn("xtable_blockmaker_t::verify_block fail-not find highqc block. proposal=%s", proposal_block->dump().c_str());
//         return enum_xtxexecutor_error_backup_tableblock_behind;
//     }
//     if (!latest_cert_block->check_block_flag(base::enum_xvblock_flag_committed)) {
//         uncommit_blocks.push_back(latest_cert_block);
//     }
//     // check latest cert block if come from lock and commit block
//     if (latest_cert_block->get_height() == (latest_blocks.get_latest_locked_block()->get_height() + 1)) {
//         if (latest_cert_block->get_last_block_hash() != latest_blocks.get_latest_locked_block()->get_block_hash()) {
//             xerror("xtable_blockmaker_t::verify_block fail-not match lock block. proposal=%s", proposal_block->dump().c_str());
//             return enum_xtxexecutor_error_backup_tableblock_behind;
//         }
//         if (!latest_blocks.get_latest_locked_block()->check_block_flag(base::enum_xvblock_flag_committed)) {
//             xblock_ptr_t lock_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
//             uncommit_blocks.push_back(lock_block);
//         }
//     }

//     return xsuccess;
// }

// int xtable_blockmaker_t::make_tableblock(const std::vector<std::string> &accounts,
//                                                        base::xvblock_t* latest_cert_block,
//                                                        const xblock_consensus_para_t & cs_para,
//                                                        uint64_t committed_height,
//                                                        xtxpool::xtxpool_table_face_t* txpool_table,
//                                                        base::xvblock_t* & proposal_block) {
//     xtableblock_proposal_input_t table_proposal_input;
//     xtable_block_para_t table_para;
//     int ret = xsuccess;
//     int32_t tableblock_batch_tx_num_residue = XGET_CONFIG(tableblock_batch_tx_max_num);
//     for (auto iter : accounts) {
//         const std::string & unit_account = iter;

//         base::xvblock_t* unit = nullptr;
//         xunit_proposal_input_t unit_proposal_input;
//         ret = m_unit_blockmaker->make_block(unit_account, cs_para, committed_height, txpool_table, unit, unit_proposal_input);
//         if (ret) {
//             xwarn("xtable_blockmaker_t::make_block fail-try make unit fail. latest_cert_block:%s %s error:%s",
//                 latest_cert_block->dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(ret).c_str());
//             continue;
//         }
//         base::xauto_ptr<base::xvblock_t> auto_release_unit(unit);  // TODO(jimmy) 后续统一改成智能指针
//         table_para.add_unit(unit);
//         data::xblock_t* block = (data::xblock_t*)unit;
//         table_proposal_input.add_unit_input(unit_proposal_input);
//         tableblock_batch_tx_num_residue -= unit_proposal_input.get_input_txs().size();
//         if (tableblock_batch_tx_num_residue <= 0) {
//             break;
//         }
//     }

//     if (table_para.get_account_units().size() == 0) {
//         return ret;
//     }

//     table_para.set_extra_data(cs_para.get_extra_data());
//     proposal_block = xtable_block_t::create_next_tableblock(table_para, latest_cert_block);
//     data::xblock_t* block = (data::xblock_t*)proposal_block;
//     block->set_consensus_para(cs_para);

//     std::string proposal_input_str = table_proposal_input.serialize_to_string();
//     block->get_input()->set_proposal(proposal_input_str);

//     XMETRICS_PACKET_INFO("consensus_tableblock", "proposal_create", proposal_block->dump(),
//         "unit_count", std::to_string(table_para.get_account_units().size()), "tx_count", std::to_string(table_proposal_input.get_total_txs()));
//     for (auto & unit : table_para.get_account_units()) {
//         // xinfo("xtable_blockmaker_t::make_block unit info. %s unit=%s %s",
//         //     proposal_block->dump().c_str(),
//         //     unit->dump_header().c_str(),
//         //     unit->dump_body().c_str());
//         XMETRICS_PACKET_INFO("consensus_tableblock", "proposal_create", proposal_block->dump(), "unit", unit->dump());
//         const auto & txs = unit->get_txs();
//         for (auto & tx : txs) {
//             XMETRICS_PACKET_INFO("consensus_tableblock", "proposal_create", proposal_block->dump(), "transaction", tx->dump());
//         }
//     }

//     return xsuccess;
// }

// int xtable_blockmaker_t::verify_out_tableblock(base::xvblock_t *proposal_block, base::xvblock_t *local_block) {
//     if (local_block->get_input_hash() != proposal_block->get_input_hash()) {
//         xwarn("xtable_blockmaker_t::verify_block input hash not match. %s %s",
//             proposal_block->dump().c_str(),
//             local_block->dump().c_str());
//         return enum_xtxexecutor_error_backup_verify_fail_table_input_hash;
//     }
//     if (local_block->get_output_hash() != proposal_block->get_output_hash()) {
//         xwarn("xtable_blockmaker_t::verify_block output hash not match. %s %s",
//             proposal_block->dump().c_str(),
//             local_block->dump().c_str());
//         return enum_xtxexecutor_error_backup_verify_fail_table_output_hash;
//     }
//     if (local_block->get_header_hash() != proposal_block->get_header_hash()) {
//         xerror("xtable_blockmaker_t::verify_block header hash not match. %s proposal:%s local:%s",
//             proposal_block->dump().c_str(),
//             ((data::xblock_t*)proposal_block)->dump_header().c_str(),
//             ((data::xblock_t*)local_block)->dump_header().c_str());
//         return enum_xtxexecutor_error_backup_verify_fail_table_header_hash;
//     }
//     if (local_block->get_cert()->get_hash_to_sign() != proposal_block->get_cert()->get_hash_to_sign()) {
//         xerror("xtable_blockmaker_t::verify_block cert hash not match. proposal:%s local:%s",
//             ((data::xblock_t*)proposal_block)->dump_cert().c_str(),
//             ((data::xblock_t*)local_block)->dump_cert().c_str());
//         return xconsensus::enum_xconsensus_error_bad_proposal;
//     }
//     bool bret = proposal_block->set_output_resources(local_block->get_output()->get_resources_data());
//     if (!bret) {
//         xerror("xtable_blockmaker_t::verify_block set proposal block output fail");
//         return xconsensus::enum_xconsensus_error_bad_proposal;
//     }
//     bret = proposal_block->set_input_resources(local_block->get_input()->get_resources_data());
//     if (!bret) {
//         xerror("xtable_blockmaker_t::verify_block set proposal block input fail");
//         return xconsensus::enum_xconsensus_error_bad_proposal;
//     }
//     return xsuccess;
// }

// std::string xtable_blockmaker_t::calc_random_seed(base::xvblock_t* latest_cert_block, base::xvqcert_t* drand_cert, uint64_t viewtoken) {
//     std::string random_str;
//     uint64_t last_block_nonce = latest_cert_block->get_cert()->get_nonce();
//     std::string drand_signature = drand_cert->get_verify_signature();
//     xassert(!drand_signature.empty());
//     random_str = base::xstring_utl::tostring(last_block_nonce);
//     random_str += base::xstring_utl::tostring(viewtoken);
//     random_str += drand_signature;
//     uint64_t seed = base::xhash64_t::digest(random_str);
//     return base::xstring_utl::tostring(seed);
// }

// // TODO(jimmy) default and transaction consensus para
// bool xtable_blockmaker_t::leader_set_consensus_para(base::xvblock_t* latest_cert_block, const xblock_maker_para_t & blockmaker_para, xblock_consensus_para_t & cs_para, bool is_empty_block) {
//     base::xvblock_t* timer_block = blockmaker_para.timer_block;
//     base::xvblock_t* drand_block = blockmaker_para.drand_block;
//     xassert(timer_block->get_height() > 0);
//     uint32_t viewtoken = base::xtime_utl::get_fast_randomu();
//     xassert(viewtoken != 0);
//     cs_para.set_common_consensus_para(timer_block->get_clock(),
//                                       blockmaker_para.validator_xip,
//                                       blockmaker_para.auditor_xip,
//                                       blockmaker_para.viewid,
//                                       viewtoken,
//                                       blockmaker_para.drand_height);
//     if (!is_empty_block) {
//         uint64_t total_lock_tgas_token = 0;
//         uint64_t property_height = 0;
//         bool ret = store::xtgas_singleton::get_instance().leader_get_total_lock_tgas_token(m_blockstore, timer_block->get_height(), total_lock_tgas_token, property_height);
//         if (!ret) {
//             xwarn("xtable_blockmaker_t::make_block fail-leader_set_consensus_para.");
//             return ret;
//         }
//         xblockheader_extra_data_t blockheader_extradata;
//         blockheader_extradata.set_tgas_total_lock_amount_property_height(property_height);
//         std::string extra_data;
//         blockheader_extradata.serialize_to_string(extra_data);
//         uint64_t timestamp = (uint64_t)(timer_block->get_clock() * 10) + base::TOP_BEGIN_GMTIME;
//         std::string random_seed = calc_random_seed(latest_cert_block, drand_block->get_cert(), viewtoken);
//         cs_para.set_tableblock_consensus_para(timestamp,
//                                               blockmaker_para.drand_height,
//                                               random_seed,
//                                               total_lock_tgas_token,
//                                               extra_data);
//         xdbg_info("xtable_blockmaker_t::set_consensus_para account=%s,height=%ld,viewtoken=%d,random_seed=%s,tgas_token=%ld,tgas_height=%ld leader",
//             latest_cert_block->get_account().c_str(), latest_cert_block->get_height(), viewtoken, random_seed.c_str(), total_lock_tgas_token, property_height);
//     }
//     return true;
// }

// bool xtable_blockmaker_t::backup_set_consensus_para(base::xvblock_t* latest_cert_block, base::xvblock_t* proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para) {
//     cs_para.set_common_consensus_para(proposal->get_cert()->get_clock(),
//                                       proposal->get_cert()->get_validator(),
//                                       proposal->get_cert()->get_auditor(),
//                                       proposal->get_cert()->get_viewid(),
//                                       proposal->get_cert()->get_viewtoken(),
//                                       proposal->get_cert()->get_drand_height());
//     if (proposal->get_block_class() == base::enum_xvblock_class_light) {
//         uint64_t property_height = 0;
//         uint64_t total_lock_tgas_token = 0;
//         xassert(!proposal->get_header()->get_extra_data().empty());
//         if (!proposal->get_header()->get_extra_data().empty()) {
//             const std::string & extra_data = proposal->get_header()->get_extra_data();
//             xblockheader_extra_data_t blockheader_extradata;
//             int32_t ret = blockheader_extradata.deserialize_from_string(extra_data);
//             if (ret <= 0) {
//                 xerror("xtable_blockmaker_t::verify_block fail-extra data invalid");
//                 return false;
//             }

//             property_height = blockheader_extradata.get_tgas_total_lock_amount_property_height();
//             bool bret = store::xtgas_singleton::get_instance().backup_get_total_lock_tgas_token(m_blockstore, proposal->get_cert()->get_clock(), property_height, total_lock_tgas_token);
//             if (!bret) {
//                 xwarn("xtable_blockmaker_t::verify_block fail-backup_set_consensus_para.");
//                 return bret;
//             }
//         }

//         std::string random_seed = calc_random_seed(latest_cert_block, bind_drand_cert, proposal->get_cert()->get_viewtoken());
//         cs_para.set_tableblock_consensus_para(proposal->get_cert()->get_gmtime(),
//                                               proposal->get_cert()->get_drand_height(),
//                                               random_seed,
//                                               total_lock_tgas_token,
//                                               proposal->get_header()->get_extra_data());
//         xdbg_info("xtable_blockmaker_t::set_consensus_para account=%s,height=%ld,viewtoken=%d,random_seed=%s,tgas_token=%ld backup",
//             latest_cert_block->get_account().c_str(), latest_cert_block->get_height(), proposal->get_cert()->get_viewtoken(),
//             random_seed.c_str(), total_lock_tgas_token);
//     }
//     return true;
// }

// base::xvblock_t* xtable_blockmaker_t::create_full_table(const xaccount_ptr_t & blockchain,
//                                     base::xvblock_t* latest_cert_block,
//                                     const std::vector<xblock_ptr_t> & uncommit_latest_blocks,
//                                     const xtable_mbt_ptr_t & last_full_table_mbt,
//                                     const xblock_consensus_para_t & cs_para) {
//     xtable_mbt_binlog_ptr_t table_mbt_binlog = blockchain->get_table_mbt_binlog();
//     xassert(table_mbt_binlog != nullptr);
//     xfulltable_block_para_t blockpara(last_full_table_mbt, table_mbt_binlog, uncommit_latest_blocks);
//     base::xvblock_t* proposal_block = xblocktool_t::create_next_fulltable(blockpara, latest_cert_block);
//     data::xblock_t* block = (data::xblock_t*)proposal_block;
//     block->set_consensus_para(cs_para);
//     return proposal_block;
// }

// int xtable_blockmaker_t::make_full_table(const base::xblock_mptrs & latest_blocks,
//                                         const xblock_consensus_para_t & cs_para,
//                                         base::xvblock_t* & proposal_block) {
//     base::xvblock_t* commit_block = latest_blocks.get_latest_committed_block();
//     xblockchain_ptr_t commit_account = m_store->query_account(commit_block->get_account());
//     xassert(commit_account != nullptr);
//     if (commit_account->get_last_height() != commit_block->get_height()) {
//         xerror("xtable_blockmaker_t::make_full_table account behind. account height %ld, commit block height %ld",
//             commit_account->get_last_height(), commit_block->get_height());
//         return enum_xtxexecutor_error_account_hehind_block;
//     }

//     xtable_mbt_ptr_t last_full_table_mbt = make_object_ptr<xtable_mbt_t>();
//     if (commit_block->get_last_full_block_height() != 0) {
//         std::string full_offstate_value = m_store->get_full_offstate(commit_block->get_account(), commit_block->get_last_full_block_height());
//         if (full_offstate_value.empty()) {
//             xerror("xtable_blockmaker_t::make_full_table full offstate load fail. account=%s, full_height=%ld",
//                 commit_block->get_account().c_str(), commit_block->get_last_full_block_height());
//             return enum_xtxexecutor_error_account_hehind_block;
//         }
//         int32_t ret = last_full_table_mbt->serialize_from_string(full_offstate_value);
//         xassert(ret > 0);
//     }

//     std::vector<xblock_ptr_t> uncommit_blocks;
//     xblock_ptr_t cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
//     xblock_ptr_t lock_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
//     uncommit_blocks.push_back(lock_block);
//     uncommit_blocks.push_back(cert_block);

//     proposal_block = create_full_table(commit_account, latest_blocks.get_latest_cert_block(), uncommit_blocks, last_full_table_mbt, cs_para);
//     return xsuccess;
// }

NS_END2
