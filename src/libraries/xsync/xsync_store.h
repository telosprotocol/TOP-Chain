// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xblock.h"
#include "xstore/xstore_face.h"
#include "xbasic/xlru_cache.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xvledger/xvblock.h"
#include "xblockstore/xsyncvstore_face.h"
#include "xsyncbase/xsync_policy.h"
#include "xmbus/xevent_ports.h"

NS_BEG2(top, sync)

class xsync_store_shadow_t;
class xsync_store_face_t {
public:
    virtual bool store_block(base::xvblock_t* block) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_connected_block(const std::string & account) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_committed_block(const std::string & account) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_locked_block(const std::string & account) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_cert_block(const std::string & account) = 0;

    virtual base::xauto_ptr<base::xvblock_t> load_block_object(const std::string & account, const uint64_t height, bool ask_full_load, uint64_t viewid = 0) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_full_block(const std::string & account) = 0;
    virtual base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &account, uint64_t height, const std::string &hash) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) = 0;
    virtual base::xauto_ptr<base::xvblock_t> get_latest_end_block(const std::string & account, enum_chain_sync_policy sync_policy) = 0;
    virtual std::vector<data::xvblock_ptr_t> load_block_objects(const std::string & account, const uint64_t height) = 0;
    virtual std::vector<data::xvblock_ptr_t> load_block_objects(const std::string & tx_hash, const base::enum_transaction_subtype type) = 0;
    virtual bool existed(const std::string & account, const uint64_t height, uint64_t viewid = 0) = 0;

    virtual void update_latest_genesis_connected_block(const std::string & account) = 0;

    virtual uint64_t get_genesis_block_height(const std::string & account) = 0;
    virtual uint64_t get_latest_committed_block_height(const std::string & account) = 0;
    virtual uint64_t get_latest_connected_block_height(const std::string & account) = 0;
    virtual uint64_t get_latest_genesis_connected_block_height(const std::string & account) = 0;
    virtual uint64_t get_latest_executed_block_height(const std::string & account) = 0;
    virtual uint64_t get_latest_start_block_height(const std::string & account, enum_chain_sync_policy sync_policy) = 0;
    virtual uint64_t get_latest_end_block_height(const std::string & account, enum_chain_sync_policy sync_policy) = 0;
    virtual uint32_t add_listener(int major_type, mbus::xevent_queue_cb_t cb) = 0;
    virtual void remove_listener(int major_type, uint32_t id) = 0;
    virtual bool set_genesis_height(const base::xvaccount_t &account, const std::string &height) = 0;
    virtual const std::string get_genesis_height(const base::xvaccount_t &account) = 0;
    virtual bool set_block_span(const base::xvaccount_t &account, const uint64_t height,  const std::string &span) = 0;
    virtual bool delete_block_span(const base::xvaccount_t &account, const uint64_t height) = 0;
    virtual const std::string get_block_span(const base::xvaccount_t &account, const uint64_t height) = 0;
    virtual xsync_store_shadow_t* get_shadow() =  0;
    const static uint64_t m_undeterministic_heights = 2;
};

class xsync_store_face_mock_t : public xsync_store_face_t {
public:
    virtual bool store_block(base::xvblock_t* block) {return false;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_connected_block(const std::string & account) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_committed_block(const std::string & account) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_locked_block(const std::string & account) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_cert_block(const std::string & account) {return nullptr;}

    virtual base::xauto_ptr<base::xvblock_t> load_block_object(const std::string & account, const uint64_t height, bool ask_full_load, uint64_t viewid = 0) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_full_block(const std::string & account) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &account, uint64_t height, const std::string &hash) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t> get_latest_end_block(const std::string & account, enum_chain_sync_policy sync_policy) {return nullptr;}
    virtual std::vector<data::xvblock_ptr_t> load_block_objects(const std::string & account, const uint64_t height) {return std::vector<data::xvblock_ptr_t>{};}
    virtual std::vector<data::xvblock_ptr_t> load_block_objects(const std::string & tx_hash, const base::enum_transaction_subtype type) {return std::vector<data::xvblock_ptr_t>{};}
    virtual bool existed(const std::string & account, const uint64_t height, uint64_t viewid = 0) {return false;}
    virtual xsync_store_shadow_t* get_shadow() {return nullptr;}
    virtual void update_latest_genesis_connected_block(const std::string & account) override;

    virtual uint64_t get_genesis_block_height(const std::string & account) override;
    virtual uint64_t get_latest_committed_block_height(const std::string & account) override;
    virtual uint64_t get_latest_connected_block_height(const std::string & account) override;
    virtual uint64_t get_latest_genesis_connected_block_height(const std::string & account) override;
    virtual uint64_t get_latest_executed_block_height(const std::string & account) override;
    virtual uint64_t get_latest_start_block_height(const std::string & account, enum_chain_sync_policy sync_policy) override;
    virtual uint64_t get_latest_end_block_height(const std::string & account, enum_chain_sync_policy sync_policy) override;
};

class xsync_store_t : public xsync_store_face_t {
public:
    xsync_store_t(std::string vnode_id, const observer_ptr<base::xvblockstore_t> &blockstore, xsync_store_shadow_t *shadow);
    bool store_block(base::xvblock_t* block) override;
    base::xauto_ptr<base::xvblock_t> get_latest_connected_block(const std::string & account) override;
    base::xauto_ptr<base::xvblock_t> get_latest_committed_block(const std::string & account) override;
    base::xauto_ptr<base::xvblock_t> get_latest_locked_block(const std::string & account) override;
    base::xauto_ptr<base::xvblock_t> get_latest_cert_block(const std::string & account) override;

    base::xauto_ptr<base::xvblock_t> load_block_object(const std::string & account, const uint64_t height, bool ask_full_load, uint64_t viewid = 0) override;
    base::xauto_ptr<base::xvblock_t> get_latest_full_block(const std::string & account) override;
    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t & account, uint64_t height, const std::string &hash) override;
    base::xauto_ptr<base::xvblock_t> get_latest_start_block(const std::string & account, enum_chain_sync_policy sync_policy) override;
    base::xauto_ptr<base::xvblock_t> get_latest_end_block(const std::string & account, enum_chain_sync_policy sync_policy) override;
    std::vector<data::xvblock_ptr_t> load_block_objects(const std::string & account, const uint64_t height) override;
    std::vector<data::xvblock_ptr_t> load_block_objects(const std::string & tx_hash, const base::enum_transaction_subtype type) override;
    bool existed(const std::string & account, const uint64_t height, uint64_t viewid = 0) override;
    virtual void update_latest_genesis_connected_block(const std::string & account) override;

    virtual uint64_t get_genesis_block_height(const std::string & account) override;
    virtual uint64_t get_latest_committed_block_height(const std::string & account) override;
    virtual uint64_t get_latest_connected_block_height(const std::string & account) override;
    virtual uint64_t get_latest_genesis_connected_block_height(const std::string & account) override;
    virtual uint64_t get_latest_executed_block_height(const std::string & account) override;
    virtual uint64_t get_latest_start_block_height(const std::string & account, enum_chain_sync_policy sync_policy) override;
    virtual uint64_t get_latest_end_block_height(const std::string & account, enum_chain_sync_policy sync_policy) override;

    virtual bool set_genesis_height(const base::xvaccount_t & account, const std::string &height) override;
    virtual const std::string get_genesis_height(const base::xvaccount_t & account) override;
    virtual bool set_block_span(const base::xvaccount_t & account, const uint64_t height,  const std::string &span) override;
    virtual bool delete_block_span(const base::xvaccount_t & account, const uint64_t height) override;
    virtual const std::string get_block_span(const base::xvaccount_t & account, const uint64_t height) override;
    virtual xsync_store_shadow_t* get_shadow() override;
    uint32_t add_listener(int major_type, mbus::xevent_queue_cb_t cb) override;
    void remove_listener(int major_type, uint32_t id) override;
private:
    std::string m_vnode_id;
    observer_ptr<base::xvblockstore_t> m_blockstore{};
    xsync_store_shadow_t *m_shadow;
};

NS_END2
