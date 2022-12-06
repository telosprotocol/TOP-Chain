// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatestore/xstatestore_access.h"
#include "xstatestore/xerror.h"

NS_BEG2(top, statestore)

xstatestore_cache_t::xstatestore_cache_t() : m_unitstate_cache(enum_max_unit_state_lru_cache_max) {

}

xtablestate_ext_ptr_t xstatestore_cache_t::get_latest_connectted_tablestate() const {
    return m_latest_connectted_tablestate;
}

data::xunitstate_ptr_t xstatestore_cache_t::get_unitstate(std::string const& block_hash) const {
    data::xunitstate_ptr_t state = nullptr;
    m_unitstate_cache.get(block_hash, state);
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, state != nullptr ? 1 : 0);
    return state;
}

void xstatestore_cache_t::set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& state) {
    m_unitstate_cache.put(block_hash, state);
    xdbg("xstatestore_cache_t::set_unitstate hash=%s,state=%s", base::xstring_utl::to_hex(block_hash).c_str(), state->get_bstate()->dump().c_str());
}

void xstatestore_cache_t::set_tablestate(uint64_t height, std::string const& block_hash, xtablestate_ext_ptr_t const& tablestate, bool is_commit) {
    m_table_states[height][block_hash] = tablestate;

    // update connectted state
    if (is_commit) {
        if (m_latest_connectted_tablestate == nullptr || height > m_latest_connectted_tablestate->get_table_state()->height()) {
            m_latest_connectted_tablestate = tablestate;
        }
    } else if (height > m_latest_connectted_tablestate->get_table_state()->height()+2) {
        auto _tablestate = get_prev_tablestate(height-1, tablestate->get_table_state()->get_bstate()->get_last_block_hash());
        if (nullptr != _tablestate) {
            m_latest_connectted_tablestate = _tablestate;
        } else {
            xassert(false);
        }
    }

    // XTODO keep four heights table states in cache
    for (auto iter = m_table_states.begin(); iter != m_table_states.end(); ) {
        if (iter->first + 3 < height) {
            iter = m_table_states.erase(iter);
        } else {
            break;
        }
    }    
}
xtablestate_ext_ptr_t xstatestore_cache_t::get_tablestate_inner(uint64_t height, std::string const& block_hash) const {
    auto iter = m_table_states.find(height);
    if (iter != m_table_states.end()) {
        const auto & hash_map = iter->second;
        auto iter2 = hash_map.find(block_hash);
        if (iter2 != hash_map.end()) {
            return iter2->second;
        }
    }
    return nullptr;
}
xtablestate_ext_ptr_t xstatestore_cache_t::get_tablestate(uint64_t height, std::string const& block_hash) const {
    xtablestate_ext_ptr_t tablestate = get_tablestate_inner(height, block_hash);
    XMETRICS_GAUGE(metrics::statestore_get_table_state_from_cache, tablestate != nullptr ? 1 : 0);
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_cache_t::get_prev_tablestate(uint64_t height, std::string const& block_hash) const {
    xtablestate_ext_ptr_t tablestate = get_tablestate_inner(height, block_hash);
    if (tablestate != nullptr) {
        return get_tablestate_inner(height, block_hash);
    }
    return nullptr;
}

//============================xstatestore_dbaccess_t============================
void xstatestore_dbaccess_t::write_table_bstate(common::xaccount_address_t const& address, data::xtablestate_ptr_t const& tablestate, const std::string & block_hash, std::error_code & ec) const {
    if (tablestate->height() > 0)
        xassert(tablestate->get_block_viewid() != 0);
    XMETRICS_GAUGE(metrics::store_state_table_write, 1);
    // 1.write table bstate to db
    std::string state_db_key = base::xvdbkey_t::create_prunable_state_key(address.vaccount(), tablestate->height(), block_hash);
    std::string state_db_bin;
    int32_t ret = tablestate->get_bstate()->serialize_to_string(state_db_bin);    
    if(ret < 0 || false == m_statestore_base.get_dbstore()->set_value(state_db_key, state_db_bin)) {
        ec = error::xerrc_t::statestore_db_write_err;
        xerror("xstatestore_dbaccess_t::write_table_bstate fail-write table bstate.state=%s", tablestate->get_bstate()->dump().c_str());
        return;
    }

    xinfo("xstatestore_dbaccess_t::write_table_bstate succ.state=%s",tablestate->get_bstate()->dump().c_str());
}

data::xtablestate_ptr_t xstatestore_dbaccess_t::read_table_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    const std::string state_db_key = base::xvdbkey_t::create_prunable_state_key(address.vaccount(), height, block_hash);
    const std::string state_db_bin = m_statestore_base.get_dbstore()->get_value(state_db_key);
    if(state_db_bin.empty()) {
        XMETRICS_GAUGE(metrics::statestore_get_table_state_from_db, 0);
        xwarn("xstatestore_dbaccess_t::read_table_bstate,fail to read from db for account=%s,height=%ld,hash=%s",
              address.to_string().c_str(),
              height,
              base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> state_ptr = base::xvblock_t::create_state_object(state_db_bin);
    if(nullptr == state_ptr) {//remove the error data for invalid data
        m_statestore_base.get_dbstore()->delete_value(state_db_key);
        xerror("xstatestore_dbaccess_t::read_table_bstate,fail invalid data at db for account=%s,height=%ld,hash=%s",
               address.to_string().c_str(),
               height,
               base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }
    if (state_ptr->get_address() != address.to_string()) {
        xerror("xstatestore_dbaccess_t::read_table_bstate,fail bad state(%s) vs ask(account:%s) ", state_ptr->dump().c_str(), address.to_string().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::statestore_get_table_state_from_db, 1);
    xdbg("xstatestore_dbaccess_t::read_table_bstate succ.account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(state_ptr.get());
    return tablestate;
}

void xstatestore_dbaccess_t::write_unit_bstate(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) const {
    XMETRICS_GAUGE(metrics::store_state_unit_write, 1);
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(unitstate->account_address().vaccount(), unitstate->height(), block_hash);
    std::string state_db_bin;
    int32_t ret = unitstate->get_bstate()->serialize_to_string(state_db_bin);
    if(ret > 0) {
        if (m_statestore_base.get_dbstore()->set_value(state_db_key, state_db_bin)) {
            xinfo("xstatestore_dbaccess_t::write_unit_bstate succ.state=%s,hash=%s",unitstate->get_bstate()->dump().c_str(),base::xstring_utl::to_hex(block_hash).c_str());
            return;
        }
    }
    ec = error::xerrc_t::statestore_db_write_err;
    xerror("xstatestore_dbaccess_t::write_unit_bstate fail.state=%s",unitstate->get_bstate()->dump().c_str());
}

data::xunitstate_ptr_t xstatestore_dbaccess_t::read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(address.vaccount(), height, block_hash);
    const std::string state_db_bin = m_statestore_base.get_dbstore()->get_value(state_db_key);
    if(state_db_bin.empty()) {
        XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_db, 0);
        xwarn("xstatestore_dbaccess_t::read_unit_bstate,fail to read from db for account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> state_ptr = base::xvblock_t::create_state_object(state_db_bin);
    if(nullptr == state_ptr) {//remove the error data for invalid data
        m_statestore_base.get_dbstore()->delete_value(state_db_key);
        xerror(
            "xstatestore_dbaccess_t::read_unit_bstate,fail invalid data at db for account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }
    if (state_ptr->get_address() != address.to_string()) {
        xerror("xstatestore_dbaccess_t::read_unit_bstate,fail bad state(%s) vs ask(account:%s) ", state_ptr->dump().c_str(), address.to_string().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_db, 1);
    xdbg("xstatestore_dbaccess_t::read_unit_bstate succ.account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(state_ptr.get());
    return unitstate;
}


//============================xstatestore_accessor_t============================
xstatestore_accessor_t::xstatestore_accessor_t() {

}

xtablestate_ext_ptr_t xstatestore_accessor_t::get_latest_connectted_table_state() const {
    return m_state_cache.get_latest_connectted_tablestate();
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_cache(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    return m_state_cache.get_tablestate(height, block_hash);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_db_inner(common::xaccount_address_t const& address, base::xvblock_t* block, bool bstate_must) const {
    auto const & stateroot = m_store_base.get_state_root_from_block(block);
    std::error_code ec;
    std::shared_ptr<state_mpt::xstate_mpt_t> mpt = state_mpt::xstate_mpt_t::create(address, stateroot, m_store_base.get_dbstore(), ec);
    if (ec) {
        xwarn("xstatestore_accessor_t::read_table_bstate_from_db fail-create mpt.block=%s", block->dump().c_str());
        return nullptr;
    }

    data::xtablestate_ptr_t table_bstate = m_dbaccess.read_table_bstate(address, block->get_height(), block->get_block_hash());
    if (nullptr == table_bstate) {
        if (stateroot.empty() || true == bstate_must) {
            xwarn("xstatestore_accessor_t::read_table_bstate_from_db fail-read table bstate.block=%s", block->dump().c_str());
            return nullptr;            
        }
    }

    xtablestate_ext_ptr_t tablestate = std::make_shared<xtablestate_ext_t>(table_bstate, mpt);
    xdbg("xstatestore_accessor_t::read_table_bstate_from_db succ-read table bstate.block=%s", block->dump().c_str());
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_db(common::xaccount_address_t const& address, base::xvblock_t* block) const {
    return read_table_bstate_from_db_inner(address, block, true);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate(common::xaccount_address_t const& address, base::xvblock_t* block) const {
    auto tablestate = m_state_cache.get_tablestate(block->get_height(), block->get_block_hash());
    if (nullptr != tablestate) {
        return tablestate;
    }
    return read_table_bstate_from_db(address, block);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_for_account_index(common::xaccount_address_t const& address, base::xvblock_t* block) const {
    xtablestate_ext_ptr_t tablestate = m_state_cache.get_tablestate(block->get_height(), block->get_block_hash());
    if (nullptr != tablestate) {
        return tablestate;
    }    
    return read_table_bstate_from_db_inner(address, block, false);
}

data::xunitstate_ptr_t xstatestore_accessor_t::read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    data::xunitstate_ptr_t unitstate = m_state_cache.get_unitstate(block_hash);
    if (nullptr != unitstate) {
        return unitstate;
    }
    return m_dbaccess.read_unit_bstate(address, height, block_hash);
}

void xstatestore_accessor_t::write_table_bstate_to_db(common::xaccount_address_t const& address, std::string const& block_hash, data::xtablestate_ptr_t const& state, std::error_code & ec) {    
    m_dbaccess.write_table_bstate(address, state, block_hash, ec);
}

void xstatestore_accessor_t::write_table_bstate_to_cache(common::xaccount_address_t const& address, uint64_t height, std::string const& block_hash, xtablestate_ext_ptr_t const& state, bool is_commit) {    
    m_state_cache.set_tablestate(height, block_hash, state, is_commit);
}

void xstatestore_accessor_t::write_unitstate_to_db(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) {
    m_dbaccess.write_unit_bstate(unitstate, block_hash, ec);
}

void xstatestore_accessor_t::write_unitstate_to_cache(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash) {
    m_state_cache.set_unitstate(block_hash, unitstate);
}


NS_END2
