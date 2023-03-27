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

xtablestate_ext_ptr_t const& xstatestore_cache_t::get_latest_connectted_tablestate() const {
    return m_latest_connectted_tablestate;
}

data::xunitstate_ptr_t xstatestore_cache_t::get_unitstate(std::string const& block_hash) const {
    xobject_ptr_t<base::xvbstate_t> bstate = nullptr;
    m_unitstate_cache.get(block_hash, bstate);
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, bstate != nullptr ? 1 : 0);
    if (bstate == nullptr) {
        return nullptr;
    }
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    return unitstate;
}

void xstatestore_cache_t::set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& state) {
    m_unitstate_cache.put(block_hash, state->get_bstate());
    xdbg("xstatestore_cache_t::set_unitstate hash=%s:%s,state=%s", base::xstring_utl::to_hex(block_hash).c_str(), base::xstring_utl::to_hex(state->get_bstate()->get_last_block_hash()).c_str(), state->get_bstate()->dump().c_str());
}

void xstatestore_cache_t::set_latest_connected_tablestate(uint64_t height, xtablestate_ext_ptr_t const& tablestate) {
    if (m_latest_connectted_tablestate == nullptr || height > m_latest_connectted_tablestate->get_table_state()->height()) {
        m_latest_connectted_tablestate = tablestate;
    }
}

void xstatestore_cache_t::set_tablestate(uint64_t height, std::string const& block_hash, xtablestate_ext_ptr_t const& tablestate, bool is_commit) {
    m_table_states[height][block_hash] = tablestate;

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
void xstatestore_dbaccess_t::write_table_bstate(common::xtable_address_t const& address, data::xtablestate_ptr_t const& tablestate, const std::string & block_hash, std::error_code & ec) const {
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

    xinfo("xstatestore_dbaccess_t::write_table_bstate succ.state=%s,size=%zu",tablestate->get_bstate()->dump().c_str(),state_db_bin.size());
}

data::xtablestate_ptr_t xstatestore_dbaccess_t::read_table_bstate(common::xtable_address_t const& address, uint64_t height, const std::string & block_hash) const {
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
    if (unitstate->height() > 0)
        xassert(unitstate->get_block_viewid() != 0);
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(unitstate->account_address().vaccount(), unitstate->height(), block_hash);
    std::string state_db_bin;
    int32_t ret = unitstate->get_bstate()->serialize_to_string(state_db_bin);
    if(ret > 0) {
        if (m_statestore_base.get_dbstore()->set_value(state_db_key, state_db_bin)) {
            xinfo("xstatestore_dbaccess_t::write_unit_bstate succ.state=%s,hash=%s,size=%zu",unitstate->get_bstate()->dump().c_str(),base::xstring_utl::to_hex(block_hash).c_str(), state_db_bin.size());
            return;
        }
    }
    ec = error::xerrc_t::statestore_db_write_err;
    xerror("xstatestore_dbaccess_t::write_unit_bstate fail.state=%s",unitstate->get_bstate()->dump().c_str());
}

int32_t xstatestore_dbaccess_t::latest_unitstate_serialize_to_string(std::string const& block_hash, std::string const& state_bin, std::string & latest_state_db_bin) {
    base::xstream_t stream(base::xcontext_t::instance());
    const int32_t begin_size = stream.size();
    stream << block_hash;
    stream << state_bin;
    latest_state_db_bin.assign((const char *)stream.data(), stream.size());
    return (stream.size() - begin_size);
}

int32_t xstatestore_dbaccess_t::latest_unitstate_serialize_from_string(std::string const& latest_state_db_bin, std::string & block_hash, std::string & state_bin) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)latest_state_db_bin.data(), (int32_t)latest_state_db_bin.size());
    const int32_t begin_size = stream.size();
    stream >> block_hash;
    stream >> state_bin;
    return (begin_size - stream.size());
}

void xstatestore_dbaccess_t::unit_bstate_to_kv(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::map<std::string, std::string> & batch_kvs, bool store_pruneable_kv, std::error_code & ec) const {
    XMETRICS_GAUGE(metrics::store_state_unit_write, 1);
    std::string state_db_bin;
    int32_t ret = unitstate->get_bstate()->serialize_to_string(state_db_bin);
    if (ret <= 0) {
        ec = error::xerrc_t::statestore_db_write_err;
        xerror("xstatestore_dbaccess_t::unit_bstate_to_kv fail.state=%s",unitstate->get_bstate()->dump().c_str());
        return;
    }
    if (store_pruneable_kv) {
        std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(unitstate->account_address().vaccount(), unitstate->height(), block_hash);
        batch_kvs.emplace(std::make_pair(state_db_key, state_db_bin));            
    } else {
        std::string latest_state_db_bin;
        ret = latest_unitstate_serialize_to_string(block_hash, state_db_bin, latest_state_db_bin);
        if (ret <=0 ) {
            ec = error::xerrc_t::statestore_db_write_err;
            xerror("xstatestore_dbaccess_t::unit_bstate_to_kv fail.state=%s",unitstate->get_bstate()->dump().c_str());
            return;
        }
        std::string latest_state_db_key = base::xvdbkey_t::create_latest_unit_state_key(unitstate->account_address().vaccount());
        batch_kvs.emplace(std::make_pair(latest_state_db_key, latest_state_db_bin));        
    }
    xdbg("xstatestore_dbaccess_t::unit_bstate_to_kv store unitstate :%s,store_pruneable_kv:%d", unitstate->get_bstate()->dump().c_str(), store_pruneable_kv);
    return;
}

void xstatestore_dbaccess_t::batch_write_unit_bstate(const std::map<std::string, std::string> & batch_kvs, std::error_code & ec) const {
    if (m_statestore_base.get_dbstore()->set_values(batch_kvs)) {
        xinfo("xstatestore_dbaccess_t::batch_write_unit_bstate succ");
        return;
    }
    ec = error::xerrc_t::statestore_db_write_err;
}

data::xunitstate_ptr_t xstatestore_dbaccess_t::read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(address.vaccount(), height, block_hash);
    const std::string state_db_bin = m_statestore_base.get_dbstore()->get_value(state_db_key);
    if(state_db_bin.empty()) {
        std::string latest_state_db_key = base::xvdbkey_t::create_latest_unit_state_key(address.vaccount());
        const std::string state_db_bin_latest = m_statestore_base.get_dbstore()->get_value(latest_state_db_key);
        if (!state_db_bin_latest.empty()) {
            std::string state_block_hash;
            std::string latest_state_str;
            auto ret = latest_unitstate_serialize_from_string(state_db_bin_latest, state_block_hash, latest_state_str);
            if (ret <= 0) {
                xerror("xstatestore_dbaccess_t::read_unit_bstate by latest fail-invalid data.account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
                return nullptr;
            }
            if (state_block_hash != block_hash) {
                XMETRICS_GAUGE(metrics::statestore_get_unit_state_by_latest_unitstate, 0);
                xdbg("xstatestore_dbaccess_t::read_unit_bstate by latest fail.account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
                return nullptr;
            }
            base::xauto_ptr<base::xvbstate_t> state_ptr_latest = base::xvblock_t::create_state_object(latest_state_str);
            if(nullptr == state_ptr_latest) {//remove the error data for invalid data
                m_statestore_base.get_dbstore()->delete_value(state_db_key);
                xerror(
                    "xstatestore_dbaccess_t::read_unit_bstate,fail invalid data at db for account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
                return nullptr;
            }
            if (state_ptr_latest->get_address() != address.to_string() || state_ptr_latest->get_block_height() != height) {
                xerror("xstatestore_dbaccess_t::read_unit_bstate,fail bad state(%s) vs ask(account:%s) ", state_ptr_latest->dump().c_str(), address.to_string().c_str());
                return nullptr;
            }
            data::xunitstate_ptr_t unitstate_latest = std::make_shared<data::xunit_bstate_t>(state_ptr_latest.get());
            XMETRICS_GAUGE(metrics::statestore_get_unit_state_by_latest_unitstate, 1);    
            xdbg("xstatestore_dbaccess_t::read_unit_bstate by latest succ.account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
            return unitstate_latest;
        }

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
xstatestore_accessor_t::xstatestore_accessor_t(common::xtable_address_t const& address) {
    base::xvtable_t * target_table = base::xvchain_t::instance().get_table(address.vaccount().get_xvid());
    m_statehub = std::make_shared<xvstatehub_t>(target_table);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::get_latest_connectted_table_state() const {
    return m_state_cache.get_latest_connectted_tablestate();
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_cache(common::xtable_address_t const& address, uint64_t height, const std::string & block_hash) const {
    return m_state_cache.get_tablestate(height, block_hash);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_db_inner(common::xtable_address_t const& address, base::xvblock_t* block, bool bstate_must) const {
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

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_db(common::xtable_address_t const& address, base::xvblock_t* block) const {
    return read_table_bstate_from_db_inner(address, block, true);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate(common::xtable_address_t const& address, base::xvblock_t* block) const {
    auto tablestate = m_state_cache.get_tablestate(block->get_height(), block->get_block_hash());
    if (nullptr != tablestate) {
        return tablestate;
    }
    return read_table_bstate_from_db(address, block);
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_for_account_index(common::xtable_address_t const& address, base::xvblock_t* block) const {
    xtablestate_ext_ptr_t tablestate = m_state_cache.get_tablestate(block->get_height(), block->get_block_hash());
    if (nullptr != tablestate) {
        return tablestate;
    }
    return read_table_bstate_from_db_inner(address, block, false);
}

data::xunitstate_ptr_t xstatestore_accessor_t::read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    data::xunitstate_ptr_t unitstate = m_state_cache.get_unitstate(block_hash);
    if (unitstate != nullptr) {
        return unitstate;
    }
    // xobject_ptr_t<base::xvbstate_t> bstate = m_statehub->get_unitstate(address.to_string(), height, block_hash);    
    // if (nullptr != bstate) {
    //     data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    //     return unitstate;
    // }
    return m_dbaccess.read_unit_bstate(address, height, block_hash);
}

void xstatestore_accessor_t::write_table_bstate_to_db(common::xtable_address_t const& address, std::string const& block_hash, data::xtablestate_ptr_t const& state, std::error_code & ec) {
    m_dbaccess.write_table_bstate(address, state, block_hash, ec);
}

void xstatestore_accessor_t::write_table_bstate_to_cache(common::xtable_address_t const& address, uint64_t height, std::string const& block_hash, xtablestate_ext_ptr_t const& state, bool is_commit) {
    m_state_cache.set_tablestate(height, block_hash, state, is_commit);

    if (is_commit) {
        m_state_cache.set_latest_connected_tablestate(height, state);
        xdbg("xstatestore_accessor_t::write_table_bstate_to_cache update 1.%s,height=%ld",address.to_string().c_str(), height);
    } else if ( (m_state_cache.get_latest_connectted_tablestate() != nullptr)
            && (height > m_state_cache.get_latest_connectted_tablestate()->get_table_state()->height()+2) ) {
        // const auto & latest_connect_state = m_state_cache.get_latest_connectted_tablestate();
        auto lock_state = m_state_cache.get_prev_tablestate(height-1, state->get_table_state()->get_bstate()->get_last_block_hash());
        if (nullptr != lock_state) {
            auto commit_state = m_state_cache.get_prev_tablestate(height-2, lock_state->get_table_state()->get_bstate()->get_last_block_hash());
            if (nullptr != commit_state) {
                m_state_cache.set_latest_connected_tablestate(height-2, commit_state);
                xdbg("xstatestore_accessor_t::write_table_bstate_to_cache update 2.%s,height=%ld",address.to_string().c_str(), height-2);
                return;
            }
        }

        // TODO(jimmy) load commit state from cache. it may happen when jump-write table state
        auto commit_block = m_store_base.get_blockstore()->load_block_object(address.vaccount(), height-2, base::enum_xvblock_flag_committed, false);
        if (nullptr != commit_block) {
            auto commit_state = read_table_bstate_from_db(address, commit_block.get());
            if (nullptr != commit_state) {
                m_state_cache.set_latest_connected_tablestate(height-2, commit_state);
                xdbg("xstatestore_accessor_t::write_table_bstate_to_cache update 3.%s,height=%ld",address.to_string().c_str(), height-2);
                return;
            }
        }
        // it may happen
        // xassert(false);
    }
}

void xstatestore_accessor_t::write_unitstate_to_db(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) {
    m_dbaccess.write_unit_bstate(unitstate, block_hash, ec);
}

void xstatestore_accessor_t::write_unitstate_to_cache(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash) {
    m_state_cache.set_unitstate(block_hash, unitstate);
    // m_statehub->set_unitstate(block_hash, unitstate->get_bstate());
}

void xstatestore_accessor_t::unit_bstate_to_kv(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::map<std::string, std::string> & batch_kvs, bool store_pruneable_kv, std::error_code & ec) const {
    m_dbaccess.unit_bstate_to_kv(unitstate, block_hash, batch_kvs, store_pruneable_kv, ec);
}

void xstatestore_accessor_t::batch_write_unit_bstate(const std::map<std::string, std::string> & batch_kvs, std::error_code & ec) const {
    m_dbaccess.batch_write_unit_bstate(batch_kvs, ec);
}

NS_END2
