// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xtimer_driver.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xversion.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xvnetwork/xvhost_base.h"

#include <memory>
#include <mutex>
#include <unordered_map>

NS_BEG2(top, vnetwork)

class xtop_basic_vhost : public xvhost_base_t {
    using base_t = xvhost_base_t;

protected:
    common::xnetwork_id_t m_network_id;
    common::xrole_type_t m_role;  // the role specified when the program launched.

    mutable std::mutex m_crypto_keys_mutex{};
    std::unordered_map<common::xnode_id_t, xcrypto_key_t<pub>> m_crypto_keys{};

    observer_ptr<time::xchain_time_face_t> m_chain_timer;
    observer_ptr<election::cache::xdata_accessor_face_t> m_election_cache_data_accessor;

public:
    xtop_basic_vhost(xtop_basic_vhost const &) = delete;
    xtop_basic_vhost & operator=(xtop_basic_vhost const &) = delete;
    xtop_basic_vhost(xtop_basic_vhost &&) = default;
    xtop_basic_vhost & operator=(xtop_basic_vhost &&) = default;
    ~xtop_basic_vhost() = default;

    /**
     * @brief Construct the xbasic_vhost_t object.
     * @param requested_role The request role when the node launched.
     * @param network_info The network info
     */
    xtop_basic_vhost(common::xnetwork_id_t const & nid,
                     observer_ptr<time::xchain_time_face_t> const & chain_timer,
                     observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor);

    std::map<common::xslot_id_t, data::xnode_info_t> members_info_of_group2(xcluster_address_t const & group_addr, common::xelection_round_t const & election_round) const override final;

    xvnode_address_t parent_group_address(xvnode_address_t const & child_addr) const override final;

    std::map<xvnode_address_t, xcrypto_key_t<pub>> crypto_keys(std::vector<xvnode_address_t> const & nodes) const override final;

    common::xnetwork_id_t const & network_id() const noexcept override final;

    common::xlogic_time_t last_logic_time() const noexcept override final;
};

using xbasic_vhost_t = xtop_basic_vhost;

NS_END2
