// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_common/xeth/xethash.h"
#include "xevm_contract_runtime/xevm_sys_crosschain_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

using evm_common::eth::xeth_header_info_t;
using evm_common::ethash::double_node_with_merkle_proof;

class xtop_evm_eth_bridge_contract : public xtop_evm_crosschain_syscontract_face<xtop_evm_eth_bridge_contract> {
public:
    xtop_evm_eth_bridge_contract() = default;
    ~xtop_evm_eth_bridge_contract() override = default;

    bool init(const xbytes_t & rlp_bytes);
    bool sync(const xbytes_t & rlp_bytes);
    bool is_known(const u256 height, const xbytes_t & hash_bytes) const;
    bool is_confirmed(const u256 height, const xbytes_t & hash_bytes) const;
    bigint get_height() const;
    void reset();

private:
    bool verify(const xeth_header_t & prev_header, const xeth_header_t & new_header, const std::vector<double_node_with_merkle_proof> & nodes) const;
    bool record(const xeth_header_t & header);
    bool rebuild(const xeth_header_t & header, const xeth_header_info_t & last_info, const xeth_header_info_t & cur_info);
    void release(const bigint number);

    // last hash @160
    h256 get_last_hash() const;
    bool set_last_hash(const h256 hash);
    // effective hashes @161
    h256 get_effective_hash(const bigint height) const;
    bool set_effective_hash(const bigint height, const h256 hash);
    bool remove_effective_hash(const bigint height);
    // all hashes @162
    std::set<h256> get_hashes(const bigint height) const;
    bool set_hashes(const bigint height, const std::set<h256> & hashes);
    bool remove_hashes(const bigint height);
    // all headers @163
    bool get_header(const h256 hash, xeth_header_t & header) const;
    bool set_header(const h256 hash, const xeth_header_t & header);
    bool remove_header(const h256 hash);
    // headers info @164
    bool get_header_info(const h256 hash, xeth_header_info_t & header_info) const;
    bool set_header_info(const h256 hash, const xeth_header_info_t & header_info);
    bool remove_header_info(const h256 hash);
};
using xevm_eth_bridge_contract_t = xtop_evm_eth_bridge_contract;

NS_END4
