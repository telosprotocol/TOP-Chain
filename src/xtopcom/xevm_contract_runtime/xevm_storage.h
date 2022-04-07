#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address_fwd.h"
#include "xevm_runner/evm_storage_base.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"

namespace top {
namespace contract_runtime {
namespace evm {

const state_accessor::properties::xproperty_type_t evm_property_type = state_accessor::properties::xproperty_type_t::bytes;

class xtop_evm_storage : public top::evm::xevm_storage_base_t {
public:
    explicit xtop_evm_storage(statectx::xstatectx_face_ptr_t const statectx) : m_statectx{statectx} {
    }
    xtop_evm_storage(xtop_evm_storage const &) = delete;
    xtop_evm_storage & operator=(xtop_evm_storage const &) = delete;
    xtop_evm_storage(xtop_evm_storage &&) = default;
    xtop_evm_storage & operator=(xtop_evm_storage &&) = default;
    virtual ~xtop_evm_storage() = default;

    xbytes_t storage_get(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto tumple = decode_key(key);
        auto unit_state = m_statectx->load_unit_state(std::get<1>(tumple).value());
        auto state_observer = make_observer(unit_state->get_bstate().get());
        state_accessor::xstate_access_control_data_t ac_data;
        state_accessor::xstate_accessor_t sa{state_observer, ac_data};
        std::error_code ec;
        auto value = sa.get_property<evm_property_type>(std::get<0>(tumple), ec);
        assert(!ec);
        top::error::throw_error(ec);
        return value;
    }
    void storage_set(xbytes_t const & key, xbytes_t const & value) override {
        xassert(m_statectx != nullptr);
        auto tumple = decode_key(key);
        auto unit_state = m_statectx->load_unit_state(std::get<1>(tumple).value());
        auto state_observer = make_observer(unit_state->get_bstate().get());
        state_accessor::xstate_access_control_data_t ac_data;
        state_accessor::xstate_accessor_t sa{state_observer, ac_data};
        std::error_code ec;
        sa.set_property<evm_property_type>(std::get<0>(tumple), value, ec);
        top::error::throw_error(ec);
        return;
    }
    void storage_remove(xbytes_t const & key) override {
        xassert(m_statectx != nullptr);
        auto tumple = decode_key(key);
        auto unit_state = m_statectx->load_unit_state(std::get<1>(tumple).value());
        auto state_observer = make_observer(unit_state->get_bstate().get());
        state_accessor::xstate_access_control_data_t ac_data;
        state_accessor::xstate_accessor_t sa{state_observer, ac_data};
        state_accessor::properties::xproperty_identifier_t property{std::get<0>(tumple), evm_property_type};
        std::error_code ec;
        sa.clear_property(property, ec);
        top::error::throw_error(ec);
        return;
    }

private:
    statectx::xstatectx_face_ptr_t m_statectx;

    std::tuple<state_accessor::properties::xtypeless_property_identifier_t, common::xaccount_address_t, xbytes_t> decode_key(xbytes_t const & raw_bytes) {
        return {};
    }
};
using xevm_storage = xtop_evm_storage;

}  // namespace evm
}  // namespace contract_runtime
}  // namespace top