﻿// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xvledger/xvledger.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xtablestate_ctx.h"
#include "xstatectx/xunitstate_ctx.h"
#include "xstatectx/xstatectx_face.h"
#include "xstatectx/xstatectx.h"
#include "xstatestore/xstatestore_face.h"

NS_BEG2(top, statectx)

xstatectx_t::xstatectx_t(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, base::xvblock_t* commit_block, const data::xtablestate_ptr_t & commit_table_state, const xstatectx_para_t & para)
: m_statectx_base(prev_block, prev_table_state, commit_block, commit_table_state, para.m_clock), m_statectx_para(para) {
    // create proposal table state for context
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = xstatectx_base_t::create_proposal_bstate(prev_block, prev_table_state->get_bstate().get(), para.m_clock);
    data::xtablestate_ptr_t proposal_table_state = std::make_shared<data::xtable_bstate_t>(proposal_bstate.get(), false);  // change to modified state
    m_table_ctx = std::make_shared<xtablestate_ctx_t>(proposal_table_state);
}

bool xstatectx_t::is_same_table(const base::xvaccount_t & addr) const {
    return get_tableid() == addr.get_short_table_id();
}

xunitstate_ctx_ptr_t xstatectx_t::find_unit_ctx(const std::string & addr, bool is_same_table) {
    if (is_same_table) {
        auto iter = m_unit_ctxs.find(addr);
        if (iter != m_unit_ctxs.end()) {
            return iter->second;
        }
    } else {
        auto iter = m_other_table_unit_ctxs.find(addr);
        if (iter != m_other_table_unit_ctxs.end()) {
            return iter->second;
        }
    }
    return nullptr;
}

void xstatectx_t::add_unit_ctx(const std::string & addr, bool is_same_table, const xunitstate_ctx_ptr_t & unit_ctx) {
    if (is_same_table) {
        m_unit_ctxs[addr] = unit_ctx;
    } else {
        m_other_table_unit_ctxs[addr] = unit_ctx;
    }
}

xunitstate_ctx_ptr_t xstatectx_t::load_unit_ctx(const base::xvaccount_t & addr) {
    xobject_ptr_t<base::xvbstate_t> bstate = nullptr;
    data::xblock_ptr_t unitblock = nullptr;
    bool is_same = is_same_table(addr);
    xunitstate_ctx_ptr_t unit_ctx = find_unit_ctx(addr.get_address(), is_same);
    if (nullptr != unit_ctx) {
        return unit_ctx;
    }
    if (is_same) {
        base::xaccount_index_t account_index;
        if (false == m_statectx_base.load_account_index(addr, account_index)) {
            xwarn("xstatectx_t::load_unit_ctx fail-load state.addr=%s", addr.get_address().c_str());
            return nullptr;
        }
        if (account_index.get_latest_unit_hash().empty()) {
            // TODO(jimmy) before fork, need unit block
            unitblock = m_statectx_base.load_inner_table_unit_block(addr);
            if (nullptr != unitblock) {
                bstate = m_statectx_base.load_proposal_block_state(addr, unitblock.get());
                if (nullptr != bstate) {
                    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get(), false);  // modify-state
                    unit_ctx = std::make_shared<xunitstate_ctx_t>(unitstate, unitblock);
                    xdbg("xstatectx_t::load_unit_ctx succ-return unit unitstate.table=%s,account=%s,index=%s", m_table_ctx->get_table_state()->get_bstate()->dump().c_str(), addr.get_account().c_str(), account_index.dump().c_str());
                }
            }
        } else {
            data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(common::xaccount_address_t(addr.get_account()), account_index);
            if (nullptr != unitstate) {
                bstate = m_statectx_base.change_to_proposal_block_state(account_index, unitstate->get_bstate().get());
                data::xunitstate_ptr_t unitstate_modify = std::make_shared<data::xunit_bstate_t>(bstate.get(), false);  // modify-state
                unit_ctx = std::make_shared<xunitstate_ctx_t>(unitstate_modify, unitblock);
                xdbg("xstatectx_t::load_unit_ctx succ-return accountindex unitstate.table=%s,account=%s,index=%s", m_table_ctx->get_table_state()->get_bstate()->dump().c_str(), addr.get_account().c_str(), account_index.dump().c_str());
            }
        }
    } else { // different table unit state is readonly
        bstate = m_statectx_base.load_different_table_unit_state(addr);
        if (nullptr != bstate) {
            data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get(), true);  // readonly-state
            unit_ctx = std::make_shared<xunitstate_ctx_t>(unitstate);
        }
    }

    if (nullptr != unit_ctx) {
        add_unit_ctx(addr.get_address(), is_same, unit_ctx);
    } else {
        xwarn("xstatectx_t::load_unit_ctx fail-load state.addr=%s,is_same=%d", addr.get_address().c_str(), is_same);
    }
    return unit_ctx;
}

data::xunitstate_ptr_t xstatectx_t::load_unit_state(const base::xvaccount_t & addr) {
    xunitstate_ctx_ptr_t unit_ctx = load_unit_ctx(addr);
    if (nullptr != unit_ctx) {
        return unit_ctx->get_unitstate();
    }
    return nullptr;
}

data::xunitstate_ptr_t xstatectx_t::load_commit_unit_state(const base::xvaccount_t & addr) {
    bool is_same = is_same_table(addr);
    data::xunitstate_ptr_t unitstate = nullptr;
    // TODO(jimmy) load from statestore and invoke sync in statestore same-table should use commit-table
    if (is_same) {
        auto bstate = m_statectx_base.load_inner_table_commit_unit_state(addr);
        if (nullptr == bstate) {
            xwarn("xstatectx_t::load_commit_unit_state fail-load commit state.addr=%s",addr.get_account().c_str());
            return nullptr;
        }
        unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get(), true);  // readonly-state
    } else {
        unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(common::xaccount_address_t(addr.get_account()));
    }
    return unitstate;
}

data::xunitstate_ptr_t xstatectx_t::load_commit_unit_state(const base::xvaccount_t & addr, uint64_t height) {
    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_committed_state(common::xaccount_address_t(addr.get_account()), height);
    return unitstate;
}

const data::xtablestate_ptr_t &  xstatectx_t::get_table_state() const {
    return m_table_ctx->get_table_state();
}

bool xstatectx_t::do_rollback() {
    bool ret = false;
    ret = get_table_state()->do_rollback();
    if (!ret) {
        xerror("xstatectx_t::do_rollback fail-table do_rollback");
        return ret;
    }
    // no need rollback table-state
    for (auto & unitctx : m_unit_ctxs) {
        ret = unitctx.second->get_unitstate()->do_rollback();
        if (!ret) {
            xerror("xstatectx_t::do_rollback fail-do_rollback");
            return ret;
        }
    }
    xdbg("xstatectx_t::do_rollback rollback table-addr %s", get_table_address().c_str());
    return true;
}

size_t xstatectx_t::do_snapshot() {
    size_t snapshot_size = 0;
    size_t _size = get_table_state()->do_snapshot();
    snapshot_size += _size;
    // no need snapshot table-state
    for (auto & unitctx : m_unit_ctxs) {
        _size = unitctx.second->get_unitstate()->do_snapshot();
        snapshot_size += _size;
    }
    return snapshot_size;
}

bool xstatectx_t::is_state_dirty() const {
    if (get_table_state()->is_state_dirty()) {
        xdbg("xstatectx_t::is_state_dirty table dirty. %s", get_table_state()->dump().c_str());
        return true;
    }
    for (auto & unitctx : m_unit_ctxs) {
        if (unitctx.second->get_unitstate()->is_state_dirty()) {
            xdbg("xstatectx_t::is_state_dirty unit dirty. %s", unitctx.second->get_unitstate()->dump().c_str());
            return true;
        }
    }
    return false;
}

std::vector<xunitstate_ctx_ptr_t> xstatectx_t::get_modified_unit_ctx() const {
    std::vector<xunitstate_ctx_ptr_t> unitctxs;
    for (auto & unitctx : m_unit_ctxs) {
        if (unitctx.second->get_unitstate()->is_state_changed()) {
            unitctxs.push_back(unitctx.second);
        }
    }
    return unitctxs;
}

xstatectx_ptr_t xstatectx_factory_t::create_latest_cert_statectx(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, base::xvblock_t* commit_block, const data::xtablestate_ptr_t & commit_table_state, const xstatectx_para_t & para) {
    auto ret = statestore::xstatestore_hub_t::instance()->execute_table_block(prev_block);
    if (!ret) {
        return nullptr;
    }
    statectx::xstatectx_ptr_t statectx_ptr = std::make_shared<statectx::xstatectx_t>(prev_block, prev_table_state, commit_block, commit_table_state, para);
    return statectx_ptr;
}

xstatectx_ptr_t xstatectx_factory_t::create_statectx(const base::xvaccount_t & table_addr, base::xvblock_t* _block) {
    if (nullptr == _block) {
        xassert(nullptr != _block);
        return nullptr;
    }

    auto cert_tablestate = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(_block);
    if (nullptr == cert_tablestate) {
        xwarn("xstatectx_factory_t::create_statectx fail-get target state.block=%s",_block->dump().c_str());
        return nullptr;
    }

    // TODO(jimmy) commit state is used for unit sync
    xobject_ptr_t<base::xvblock_t> _commit_block = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block(table_addr);
    if (nullptr == _commit_block) {
        xerror("xstatectx_factory_t::create_statectx fail-get commit block.");
        return nullptr;
    }

    auto commit_tablestate = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(_commit_block.get());
    if (nullptr == commit_tablestate) {
        xwarn("xstatectx_factory_t::create_statectx fail-get commit state.block=%s",_commit_block->dump().c_str());
        return nullptr;
    }

    xstatectx_para_t statectx_para(_block->get_clock()+1);
    statectx::xstatectx_ptr_t statectx_ptr = std::make_shared<statectx::xstatectx_t>(_block, cert_tablestate, _commit_block.get(), commit_tablestate, statectx_para);
    return statectx_ptr;
}

NS_END2
