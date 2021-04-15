#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xcontract_execution_stage.h"
#include "xcontract_common/xcontract_execution_result.h"
#include "xdata/xtransaction.h"

#include <map>
#include <string>

NS_BEG2(top, contract_common)

class xtop_contract_execution_context {
private:
    observer_ptr<xcontract_state_t> m_contract_state{};
    xobject_ptr_t<data::xtransaction_t> m_tx;
    std::map<std::string, xbyte_buffer_t> m_receipt_data; // input receipt

    xcontract_execution_stage_t m_execution_stage{xcontract_execution_stage_t::invalid};
    xcontract_execution_result_t m_execution_result; // execution result


public:
    xtop_contract_execution_context() = default;
    xtop_contract_execution_context(xtop_contract_execution_context const &) = delete;
    xtop_contract_execution_context & operator=(xtop_contract_execution_context const &) = delete;
    xtop_contract_execution_context(xtop_contract_execution_context &&) = default;
    xtop_contract_execution_context & operator=(xtop_contract_execution_context &&) = default;
    ~xtop_contract_execution_context() = default;

    xtop_contract_execution_context(xobject_ptr_t<data::xtransaction_t> tx, observer_ptr<xcontract_state_t> s) noexcept;

    observer_ptr<xcontract_state_t> contract_state() const noexcept;
    xcontract_execution_stage_t execution_stage() const noexcept;
    void execution_stage(xcontract_execution_stage_t const stage) noexcept;
    xcontract_execution_result_t execution_result() const noexcept;
    void add_followup_transaction(data::xtransaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type);

    void receipt_data(std::map<std::string, xbyte_buffer_t> receipt_data);
    std::map<std::string, xbyte_buffer_t> & receipt_data() noexcept;
    xbyte_buffer_t const & receipt_data(std::string const & key, std::error_code & ec) const noexcept;

    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;
    common::xaccount_address_t contract_address() const;

    data::enum_xtransaction_type transaction_type() const noexcept;

    std::string action_name() const;
    data::enum_xaction_type action_type() const;
    xbyte_buffer_t action_data() const;
};
using xcontract_execution_context_t = xtop_contract_execution_context;

NS_END2
