// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xaccount_address.h"
#include "xevm_common/trie/xtrie_kv_db_face.h"
#include "xstatistic/xstatistic.h"
#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

struct xtop_state_object : public xstatistic::xstatistic_obj_face_t {
private:
    xtop_state_object();
    xtop_state_object(xtop_state_object const &);
    xtop_state_object(xtop_state_object &&);
public:
    xtop_state_object(common::xaccount_address_t const & account, const base::xaccount_index_t & new_index);
    ~xtop_state_object();

    static std::shared_ptr<xtop_state_object> new_object(common::xaccount_address_t const & account, const base::xaccount_index_t & new_index);

    base::xaccount_index_t get_account_index();
    void set_account_index(const base::xaccount_index_t & new_index);

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_mpt_state_object;}

    common::xaccount_address_t account;
    base::xaccount_index_t index;

private:
    virtual size_t get_object_size_real() const override;
};
using xstate_object_t = xtop_state_object;

}  // namespace state_mpt
}  // namespace top