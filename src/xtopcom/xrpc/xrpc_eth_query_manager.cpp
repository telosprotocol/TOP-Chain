#include "xrpc_eth_query_manager.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <trezor-crypto/sha3.h>
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>

#include "xbase/xbase.h"
#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_network_result_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtx_factory.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
//#include "xdata/xslash.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtableblock.h"
#include "xdata/xtransaction_cache.h"
#include "xevm_common/fixed_hash.h"
#include "xevm_common/common_data.h"
#include "xevm_common/common.h"
#include "xevm_common/rlp.h"
#include "xevm_common/address.h"
#include "xrouter/xrouter.h"
#include "xrpc/xuint_format.h"
#include "xrpc/xrpc_loader.h"
#include "xstore/xaccount_context.h"
#include "xstore/xtgas_singleton.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"
#include "xmbus/xevent_behind.h"
#include "xdata/xblocktool.h"
#include "xpbase/base/top_utils.h"
#include "xutility/xhash.h"

#include "xtxexecutor/xvm_face.h"
#include "xevm/xevm.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"
#include "xstatectx/xstatectx.h"
using namespace top::data;

namespace top {
namespace xrpc {
using namespace std;
using namespace base;
using namespace store;
using namespace xrpc;

void xrpc_eth_query_manager::call_method(std::string strMethod, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    auto iter = m_query_method_map.find(strMethod);
    if (iter != m_query_method_map.end()) {
        (iter->second)(js_req, js_rsp, strResult, nErrorCode);
    }
}

bool xrpc_eth_query_manager::handle(std::string & strReq, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string action = js_req["action"].asString();
    auto iter = m_query_method_map.find(action);
    if (iter != m_query_method_map.end()) {
        iter->second(js_req, js_rsp, strResult, nErrorCode);
    } else {
        xinfo("get_block action:%s not exist!", action.c_str());
        strResult = "Method not Found!";
        return false;
    }

    return true;
}
void xrpc_eth_query_manager::deal_error(xJson::Value & js_rsp, eth::enum_error_code error_id) {
    eth::ErrorMessage err_msg;
    if (m_eth_error_info.get_error(error_id, err_msg) == false) {
        xwarn("xrpc_eth_query_manager::deal_error, not find:%d", (int32_t)error_id);
        return;
    }
    js_rsp["error"]["code"] = err_msg.m_code;
    js_rsp["error"]["message"] = err_msg.m_message;
    return;
}
bool xrpc_eth_query_manager::check_eth_address(const std::string& account) {
    if (account.size() < 2)
        return false;
    if (account.substr(0,2) != "0x" && account.substr(0,2) != "0X")
        return false;
    return true;
}
xaccount_ptr_t xrpc_eth_query_manager::query_account_by_number(const std::string &unit_address, const std::string& table_height) {
    base::xvaccount_t _vaddr(unit_address);
    std::string table_address = base::xvaccount_t::make_table_account_address(_vaddr);
    base::xvaccount_t _table_addr(table_address);

    XMETRICS_GAUGE(metrics::blockstore_access_from_store, 1);
    xobject_ptr_t<base::xvblock_t> _block;
    if (table_height == "latest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else if (table_height == "earliest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(_table_addr);
    else if (table_height == "pending")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr);
    else {
        uint64_t height = std::strtoul(table_height.c_str(), NULL, 16);
        _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_table_addr, height, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_rpc_get_block_query_propery);
    }

    if (_block == nullptr) {
        xwarn("xstore::query_account_by_number fail-load. account=%s", unit_address.c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get(), metrics::statestore_access_from_vnodesrv_load_state);
    if (bstate == nullptr) {
        xwarn("xstore::query_account_by_number fail-load state.block=%s", _block->dump().c_str());
        return nullptr;
    }

    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());
    base::xaccount_index_t account_index;
    tablestate->get_account_index(unit_address, account_index);
    uint64_t unit_height = account_index.get_latest_unit_height();
    uint64_t unit_view_id = account_index.get_latest_unit_viewid();

    auto unit_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(
        _vaddr, unit_height, unit_view_id, false, metrics::blockstore_access_from_rpc_get_block_query_propery);
    if (unit_block == nullptr) {
        xwarn("xstore::query_account_by_number fail-load. account=%s", unit_address.c_str());
        return nullptr;
    }
    base::xauto_ptr<base::xvbstate_t> unit_state =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit_block.get(), metrics::statestore_access_from_vnodesrv_load_state);
    if (unit_state == nullptr) {
        xwarn("xstore::query_account_by_number fail-load state.block=%s", unit_block->dump().c_str());
        return nullptr;
    }
    return std::make_shared<xunit_bstate_t>(unit_state.get());
}
void xrpc_eth_query_manager::eth_getBalance(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    if (!check_eth_address(account)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    account = xvaccount_t::to_evm_address(account);
    xdbg("xarc_query_manager::getBalance account: %s,%s", account.c_str(), js_req[1].asString().c_str());

    ETH_ADDRESS_CHECK_VALID(account)

    xaccount_ptr_t account_ptr = query_account_by_number(account, js_req[1].asString());

    if (account_ptr == nullptr) {
        js_rsp["result"] = "0x0";
    } else {
        evm_common::u256 balance = account_ptr->tep_token_balance(data::XPROPERTY_ASSET_ETH);

        std::string balance_str = toHex((top::evm_common::h256)balance);

        uint32_t i = 0;
        for (; i < balance_str.size() - 1; i++) {
            if (balance_str[i] != '0') {
                break;
            }
        }
        js_rsp["result"] = "0x" + balance_str.substr(i);
        xdbg("xarc_query_manager::getBalance account: %s, balance:%s", account.c_str(), balance_str.substr(i).c_str());
    }
}
void xrpc_eth_query_manager::eth_getTransactionCount(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    if (!check_eth_address(account)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    account = xvaccount_t::to_evm_address(account);

    // add top address check
    ETH_ADDRESS_CHECK_VALID(account)
    xaccount_ptr_t account_ptr = query_account_by_number(account, js_req[1].asString());
    if (account_ptr == nullptr) {
        js_rsp["result"] = "0x0";
    } else {
        uint64_t nonce = account_ptr->get_latest_send_trans_number();
        xdbg("xarc_query_manager::eth_getTransactionCount: %s, %llu", account.c_str(), nonce);
        std::stringstream outstr;
        outstr << "0x" << std::hex << nonce;
        js_rsp["result"] = std::string(outstr.str());
    }
}
void xrpc_eth_query_manager::eth_getTransactionByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash = js_req[0].asString();
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getTransactionByHash tx hash: %s",  tx_hash.c_str());

    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_tx_indx_detail(tx_hash_str, base::enum_transaction_subtype_send);
    if (sendindex == nullptr) {
        xwarn("xrpc_eth_query_manager::eth_getTransactionByHash fail.tx hash:%s", tx_hash.c_str());
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    xJson::Value js_result;
    std::stringstream outstr;
    outstr << "0x" << std::hex << sendindex->get_txindex()->get_block_height();

    js_result["blockHash"] = std::string("0x") + top::HexEncode(sendindex->get_txindex()->get_block_hash());
    js_result["blockNumber"] = outstr.str();
    js_result["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
    js_result["gas"] = "0x40276";
    js_result["gasPrice"] = "0xb2d05e00";
    js_result["hash"] = tx_hash;
    js_result["input"] = "0x" + data::to_hex_str(sendindex->get_raw_tx()->get_data());
    uint64_t nonce = sendindex->get_raw_tx()->get_last_nonce();
    std::stringstream outstr_nonce;
    outstr_nonce << "0x" << std::hex << nonce;
    js_result["nonce"] = outstr_nonce.str();

    uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
    js_result["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
    if (tx_type == xtransaction_type_transfer) {
        js_result["to"] = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
    } else {
        js_result["to"] = xJson::Value::null;
    }
    js_result["transactionIndex"] = "0x0";
    std::stringstream outstr_type;
    outstr_type << "0x" << std::hex << sendindex->get_raw_tx()->get_eip_version();
    js_result["type"] = std::string(outstr_type.str());
    js_result["value"] = "0x0";

    std::string str_v = sendindex->get_raw_tx()->get_SignV();
    uint32_t i = 0;
    for (; i < str_v.size() - 1; i++) {
        if (str_v[i] != '0') {
            break;
        }
    }
    js_result["v"] = std::string("0x") + str_v.substr(i);
    js_result["r"] = std::string("0x") + sendindex->get_raw_tx()->get_SignR();
    js_result["s"] = std::string("0x") + sendindex->get_raw_tx()->get_SignS();

    js_rsp["result"] = js_result;
    xdbg("xrpc_eth_query_manager::eth_getTransactionByHash ok.tx hash:%s", tx_hash.c_str());
    return;
}

top::evm_common::h2048 xrpc_eth_query_manager::calculate_bloom(const std::string & hexstr) {
    top::evm_common::h2048 bloom;
    char szDigest[32] = {0};
    std::string str = HexDecode(hexstr);
    keccak_256((const unsigned char *)str.data(), str.size(), (unsigned char *)szDigest);
    top::evm_common::h256 hash_h256;
    top::evm_common::bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
    bloom.shiftBloom<3>(hash_h256);
    return bloom;
}

void xrpc_eth_query_manager::eth_getTransactionReceipt(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash = js_req[0].asString();
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getTransactionReceipt tx hash: %s",  tx_hash.c_str());

    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_tx_indx_detail(tx_hash_str, base::enum_transaction_subtype_send);
    if (sendindex == nullptr) {
        xwarn("xrpc_query_manager::eth_getTransactionReceipt load tx index fail.%s", tx_hash.c_str());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        js_rsp["result"] = xJson::Value::null;
        return;
    }

    xJson::Value js_result;
    js_result["transactionHash"] = tx_hash;
    std::string block_hash = std::string("0x") + top::HexEncode(sendindex->get_txindex()->get_block_hash());
    js_result["blockHash"] = block_hash;
    std::string tx_idx = "0x0";
    js_result["transactionIndex"] = tx_idx;
    std::stringstream outstr;
    outstr << "0x" << std::hex << sendindex->get_txindex()->get_block_height();
    std::string block_num = outstr.str();
    js_result["blockNumber"] = block_num;

    uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
    js_result["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
    if (tx_type == xtransaction_type_transfer) {
        js_result["to"] = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
        js_result["status"] = "0x1";
        js_result["cumulativeGasUsed"] = "0x0";
        js_result["effectiveGasPrice"] = "0x0";
        js_result["gasUsed"] = "0x0";
    } else {
        evm_common::xevm_transaction_result_t evm_result;
        auto ret = sendindex->get_txaction().get_evm_transaction_result(evm_result);

        if (tx_type == xtransaction_type_run_contract) {
            std::string contract_addr  = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
            js_result["to"] = contract_addr;
            js_result["contractAddress"] = contract_addr;
        } else {
            js_result["to"] = xJson::Value::null;
            std::string contract_addr  = evm_result.extra_msg;
            js_result["contractAddress"] = contract_addr;
        }

        evm_common::h2048 logs_bloom;
        uint32_t index = 0;
        for (auto & log : evm_result.logs) {
            xJson::Value js_log;

            std::stringstream outstr1;
            outstr1 << "0x" << std::hex << index;
            js_log["logIndex"] = outstr1.str();
            js_log["blockNumber"] = block_num;
            js_log["blockHash"] = block_hash;
            js_log["transactionHash"] = tx_hash;
            js_log["transactionIndex"] = tx_idx;
            js_log["address"] = log.address;

            evm_common::h2048 bloom = calculate_bloom(log.address);
            logs_bloom |= bloom;

            for (auto & topic : log.topics) {
                js_log["topics"].append(topic);
                evm_common::h2048 topic_bloom = calculate_bloom(topic);
                logs_bloom |= topic_bloom;
            }

            js_log["data"] = log.data;
            js_log["removed"] = false;
            js_result["logs"].append(js_log);
            index++;
        }
        if (evm_result.logs.empty()) {
            js_result["logs"].resize(0);
        }

        std::stringstream outstrbloom;
        outstrbloom << logs_bloom;
        js_result["logsBloom"] = std::string("0x") + outstrbloom.str();
        js_result["status"] = (evm_result.status == 0) ?  "0x1" : "0x0";
        std::stringstream outstr;
        outstr << "0x" << std::hex << sendindex->get_raw_tx()->get_eip_version();
        js_result["type"] = std::string(outstr.str());
        outstr.seekp(0);
        outstr << "0x" << std::hex << evm_result.used_gas;
        js_result["gasUsed"] = outstr.str();
        js_result["cumulativeGasUsed"] = outstr.str();
        js_result["effectiveGasPrice"] = "0x77359400";
    }

    js_rsp["result"] = js_result;

    xdbg("xrpc_query_manager::eth_getTransactionReceipt ok.tx hash:%s", js_req[0].asString().c_str());
    return;
}
void xrpc_eth_query_manager::eth_blockNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    uint64_t height = m_block_store->get_latest_committed_block_height(_vaddress);

    std::stringstream outstr;
    outstr << "0x" << std::hex << height;
    js_rsp["result"] = std::string(outstr.str());
    xdbg("xarc_query_manager::eth_blockNumber: %llu", height);
}

void xrpc_eth_query_manager::eth_getBlockByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint256_t hash = top::data::hex_to_uint256(js_req[0].asString());
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("eth_getBlockByHash tx hash: %s",  top::HexEncode(tx_hash_str).c_str());

    base::xauto_ptr<base::xvblock_t>  block = m_block_store->get_block_by_hash(tx_hash_str);
    if (block == nullptr)
        return;

    xJson::Value js_result;
    set_block_result(block, js_result, js_req[1].asBool());
    js_rsp["result"] = js_result;
    return;
}
void xrpc_eth_query_manager::eth_getBlockByNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint64_t height = std::strtoul(js_req[0].asString().c_str(), NULL, 16);
    xdbg("eth_getBlockByNumber: %s, %llu",  sys_contract_eth_table_block_addr, height);
    base::xvaccount_t account(std::string(sys_contract_eth_table_block_addr) + "@0");
    base::xauto_ptr<base::xvblock_t>  block = m_block_store->load_block_object(account, height, base::enum_xvblock_flag_committed, false);
    if (block == nullptr)
        return;

    xJson::Value js_result;
    set_block_result(block, js_result, js_req[1].asBool());
    js_result["baseFeePerGas"] = "0x10";
    js_rsp["result"] = js_result;
    return;
}
void xrpc_eth_query_manager::set_block_result(const base::xauto_ptr<base::xvblock_t>&  block, xJson::Value& js_result, bool fullTx) {
    js_result["difficulty"] = "0x0";
    js_result["extraData"] = "0x0";
    js_result["gasLimit"] = "0x0";
    js_result["gasUsed"] = "0x0";
    std::string hash = block->get_block_hash();
    js_result["hash"] = std::string("0x") + top::HexEncode(hash); //js_req["tx_hash"].asString();
    js_result["logsBloom"] = "0x0";
    js_result["miner"] = std::string("0x") + std::string(40, '0');
    js_result["mixHash"] = "0x0";
    js_result["nonce"] = "0x0";
    std::stringstream outstr;
    outstr << "0x" << std::hex << block->get_height();
    js_result["number"] = std::string(outstr.str());
    js_result["parentHash"] = std::string("0x") + top::HexEncode(block->get_last_block_hash());
    js_result["receiptsRoot"] = "0x0";
    js_result["sha3Uncles"] = "0x0";
    js_result["size"] = "0x0";
    js_result["stateRoot"] = "0x0";
    outstr.seekp(0);
    outstr << "0x" << std::hex << block->get_timestamp();
    js_result["timestamp"] = std::string(outstr.str());
    js_result["totalDifficulty"] = "0x0";
    js_result["transactionsRoot"] = "0x0";
    js_result["transactions"].resize(0);

    const std::vector<base::xvaction_t> input_actions = block->get_tx_actions();
    for(auto action : input_actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }
        if (!fullTx) {
            js_result["transactions"].append(std::string("0x") + to_hex_str(action.get_org_tx_hash()));
        } else {
            xJson::Value js_tx;
            js_tx["hash"] = std::string("0x") + to_hex_str(action.get_org_tx_hash());

            xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_tx_indx_detail(action.get_org_tx_hash(), base::enum_transaction_subtype_send);
            if (sendindex == nullptr) {
                xwarn("xrpc_eth_query_manager::set_block_result fail.tx hash:%s", to_hex_str(action.get_org_tx_hash()).c_str());
//                js_rsp["result"] = xJson::Value::null;
                continue;
            }

            std::string block_hash = std::string("0x") + top::HexEncode(sendindex->get_txindex()->get_block_hash());
            js_tx["blockHash"] = block_hash;
            std::stringstream outstr;
            outstr << "0x" << std::hex << sendindex->get_txindex()->get_block_height();
            std::string block_num = outstr.str();
            js_tx["blockNumber"] = block_num;

            js_tx["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
            js_tx["gas"] = "0x40276";
            js_tx["gasPrice"] = "0xb2d05e00";
            js_tx["input"] = "0x" + data::to_hex_str(sendindex->get_raw_tx()->get_data());
            uint64_t nonce = sendindex->get_raw_tx()->get_last_nonce();
            std::stringstream outstr_nonce;
            outstr_nonce << "0x" << std::hex << nonce;
            js_tx["nonce"] = outstr_nonce.str();

            uint16_t tx_type = sendindex->get_raw_tx()->get_tx_type();
            js_tx["from"] = std::string("0x") + sendindex->get_raw_tx()->get_source_addr().substr(6);
            if (tx_type == xtransaction_type_transfer) {
                js_tx["to"] = std::string("0x") + sendindex->get_raw_tx()->get_target_addr().substr(6);
            } else {
                js_tx["to"] = xJson::Value::null;
            }
            js_tx["transactionIndex"] = "0x0";
            std::stringstream outstr_type;
            outstr_type << "0x" << std::hex << sendindex->get_raw_tx()->get_eip_version();
            js_tx["type"] = std::string(outstr_type.str());
            js_tx["value"] = "0x0";

            std::string str_v = sendindex->get_raw_tx()->get_SignV();
            uint32_t i = 0;
            for (; i < str_v.size() - 1; i++) {
                if (str_v[i] != '0') {
                    break;
                }
            }
            js_tx["v"] = std::string("0x") + str_v.substr(i);
            js_tx["r"] = std::string("0x") + sendindex->get_raw_tx()->get_SignR();
            js_tx["s"] = std::string("0x") + sendindex->get_raw_tx()->get_SignS();

            js_result["transactions"].append(js_tx);
        }
    }    
}
void xrpc_eth_query_manager::eth_getCode(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    if (!check_eth_address(account)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    account = xvaccount_t::to_evm_address(account);

    // add top address check
    ETH_ADDRESS_CHECK_VALID(account)

    xaccount_ptr_t account_ptr = query_account_by_number(account, js_req[1].asString());
    if (account_ptr == nullptr) {
        js_rsp["result"] = "0x";
        xdbg("xrpc_eth_query_manager::eth_getCode account: %s", account.c_str());
    } else {
        std::string code = account_ptr->get_code();
        code = std::string("0x") + top::HexEncode(code);
        xdbg("xrpc_eth_query_manager::eth_getCode account: %s, %s", account.c_str(), code.c_str());
        js_rsp["result"] = code;
    }
    return;
}
std::string xrpc_eth_query_manager::safe_get_json_value(xJson::Value & js_req, const std::string& key) {
    if (js_req.isMember(key))
        return js_req[key].asString();
    return "";
}
void xrpc_eth_query_manager::eth_call(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string to = safe_get_json_value(js_req[0], "to");
    if (!check_eth_address(to)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    to = xvaccount_t::to_evm_address(to);
    ETH_ADDRESS_CHECK_VALID(to)

    std::string from = safe_get_json_value(js_req[0], "from");
    if (!check_eth_address(from)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    from = xvaccount_t::to_evm_address(from);
    ETH_ADDRESS_CHECK_VALID(from)

    std::string jdata = safe_get_json_value(js_req[0], "data");
    std::string data;
    if (jdata.size() > 2)
        data = top::HexDecode(jdata.substr(2));
    std::string value = safe_get_json_value(js_req[0], "value");
    std::string gas = safe_get_json_value(js_req[0], "gas");
    std::string gas_price = safe_get_json_value(js_req[0], "gasPrice");
    top::evm_common::u256 gas_value;
    if (gas.empty() || gas_price.empty())
        gas_value = (uint64_t)12000000U;
    else
        gas_value = std::strtoul(gas.c_str(), NULL, 16) * std::strtoul(gas_price.c_str(), NULL, 16);
    top::data::xtransaction_ptr_t tx = top::data::xtx_factory::create_ethcall_v3_tx(from, to, data, std::strtoul(value.c_str(), NULL, 16), gas_value);
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());
    xinfo("xrpc_eth_query_manager::eth_call, %s, %s, %s", jdata.c_str(), value.c_str(), gas_value.str().c_str());

    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    auto block = m_block_store->get_latest_committed_block(_vaddress);   
    uint64_t gmtime = block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(addr, block->get_clock(), block->get_viewid(), block->get_viewtoken(), block->get_height(), gmtime);

    statectx::xstatectx_ptr_t statectx_ptr = statectx::xstatectx_factory_t::create_latest_commit_statectx(_vaddress);
    if (statectx_ptr == nullptr) {
        xwarn("create_latest_commit_statectx fail: %s", addr.c_str());
        return;
    }
    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token());

    txexecutor::xvm_input_t input{statectx_ptr, vmpara, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{nullptr, statectx_ptr};

    auto ret = evm.execute(input, output);
    if (ret != txexecutor::enum_exec_success) {
        xwarn("evm call fail.");
        return;
    }
    xinfo("evm call: %d, %s", output.m_tx_result.status, output.m_tx_result.extra_msg.c_str());
    if (output.m_tx_result.status == 0)
        js_rsp["result"] = output.m_tx_result.extra_msg;
}

void xrpc_eth_query_manager::eth_estimateGas(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string to = safe_get_json_value(js_req[0], "to");
    if (to.empty()) {
        xinfo("xrpc_eth_query_manager::eth_estimateGas, generate_tx to: %s", to.c_str());
        to = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + std::string(40, '0');
    } else {
        to = xvaccount_t::to_evm_address(to);
    }
    ETH_ADDRESS_CHECK_VALID(to)

    std::string from = safe_get_json_value(js_req[0], "from");
    if (!check_eth_address(from)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    from = xvaccount_t::to_evm_address(from);
    ETH_ADDRESS_CHECK_VALID(from)

    std::string jdata = safe_get_json_value(js_req[0], "data");
    std::string data;
    if (jdata.size() > 2)
        data = top::HexDecode(jdata.substr(2));
    std::string value = safe_get_json_value(js_req[0], "value");
    std::string gas = safe_get_json_value(js_req[0], "gas");
    std::string gas_price = safe_get_json_value(js_req[0], "gasPrice");
    top::evm_common::u256 gas_value;
    if (gas.empty() || gas_price.empty())
        gas_value = (uint64_t)12000000U;
    else
        gas_value = std::strtoul(gas.c_str(), NULL, 16) * std::strtoul(gas_price.c_str(), NULL, 16);
    top::data::xtransaction_ptr_t tx = top::data::xtx_factory::create_ethcall_v3_tx(from, to, data, std::strtoul(value.c_str(), NULL, 16), gas_value);
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());
    xinfo("xrpc_eth_query_manager::eth_estimateGas, %s, %s, %s", jdata.c_str(), value.c_str(), gas_value.str().c_str());

    std::string addr = std::string(sys_contract_eth_table_block_addr) + "@0";
    base::xvaccount_t _vaddress(addr);
    auto block = m_block_store->get_latest_committed_block(_vaddress);   
    uint64_t gmtime = block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(addr, block->get_clock(), block->get_viewid(), block->get_viewtoken(), block->get_height(), gmtime);

    statectx::xstatectx_ptr_t statectx_ptr = statectx::xstatectx_factory_t::create_latest_commit_statectx(_vaddress);
    if (statectx_ptr == nullptr) {
        xwarn("create_latest_commit_statectx fail: %s", addr.c_str());
        return;
    }

    base::xvaccount_t _vaddr(from);
    data::xunitstate_ptr_t unitstate = statectx_ptr->load_unit_state(_vaddr);
    if (nullptr == unitstate) {
        xwarn("eth_estimateGas fail-load unit state, %s", from.c_str());
        return;
    }
    unitstate->tep_token_deposit(data::XPROPERTY_ASSET_ETH, gas_value);

    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token());

    txexecutor::xvm_input_t input{statectx_ptr, vmpara, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{nullptr, statectx_ptr};

    auto ret = evm.execute(input, output);
    if (ret != txexecutor::enum_exec_success) {
        xwarn("evm call fail.");
        return;
    }
    xinfo("eth_estimateGas call: %d, %s, %llu", output.m_tx_result.status, output.m_tx_result.extra_msg.c_str(), output.m_tx_result.used_gas);
    if (output.m_tx_result.status == 0) {
        std::stringstream outstr;
        outstr << "0x" << std::hex << output.m_tx_result.used_gas;        
        js_rsp["result"] = outstr.str();
    }
}
void xrpc_eth_query_manager::eth_getStorageAt(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req[0].asString();
    if (!check_eth_address(account)) {
        deal_error(js_rsp, eth::enum_invalid_argument_hex);
        return;
    }
    account = xvaccount_t::to_evm_address(account);
    xdbg("xarc_query_manager::eth_getStorageAt account: %s,%s", account.c_str(), js_req[1].asString().c_str());

    ETH_ADDRESS_CHECK_VALID(account)

    xunitstate_ptr_t account_ptr = query_account_by_number(account, js_req[2].asString());
    if (account_ptr == nullptr) {
        js_rsp["result"] = "0x0";
        return;
    }

    std::string index_str = top::HexDecode(js_req[1].asString().substr(2));
    std::string value_str = account_ptr->get_storage(index_str);
    xinfo("xrpc_eth_query_manager::eth_getStorageAt, %s,%s,%s, %s", js_req[0].asString().c_str(), js_req[1].asString().c_str(), js_req[2].asString().c_str(),
        top::HexEncode(value_str).c_str());
    js_rsp["result"] = std::string("0x") + top::HexEncode(value_str);
}
}  // namespace chain_info
}  // namespace top
