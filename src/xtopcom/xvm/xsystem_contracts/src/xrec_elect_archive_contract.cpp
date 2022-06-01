// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_archive_contract.h"

#include "xbasic/xutility.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xnode_id.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv1/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xelection/xv1/xstandby_result_store.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xserialization/xserialization.h"

#include <cinttypes>

#ifdef STATIC_CONSENSUS
#   include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"
#   include "xdata/xelection/xelection_info.h"
#   include "xdata/xelection/xelection_info_bundle.h"
#endif

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "RecElectArchive_"
#define XARCHIVE_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, rec)

using common::xnode_id_t;
using data::election::xelection_result_store_t;
using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;

xtop_rec_elect_archive_contract::xtop_rec_elect_archive_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

#ifdef STATIC_CONSENSUS
bool executed_archive{false};
// if enabled static_consensus
// make sure add config in config.xxxx.json
// like this :
//
// "archive_start_nodes":"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp.0.pub_key,T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446.0.pub_key,T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB.0.pub_key",
//
// it will elect the first and only round archive nodes as you want.

void xtop_rec_elect_archive_contract::elect_config_nodes(common::xlogic_time_t const current_time) {
    uint64_t latest_height = get_blockchain_height(sys_contract_rec_elect_archive_addr);
    xinfo("[archive_start_nodes] get_latest_height: %" PRIu64, latest_height);
    if (latest_height > 0) {
        executed_archive = true;
        return;
    }

    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xstandby_node_info_t;

    auto property = data::election::get_property_by_group_id(common::xarchive_group_id);

    auto election_result_store = xvm::serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, property);
    auto & election_network_result = election_result_store.result_of(network_id());
    auto node_type = common::xnode_type_t::storage_archive;

    auto nodes_info = xstatic_election_center::instance().get_static_election_nodes("archive_start_nodes");
    if (nodes_info.empty()) {
        xinfo("[archive_start_nodes] get empty node_info");
        return;
    }
    auto & election_group_result = election_result_store.result_of(network_id()).result_of(node_type).result_of(common::xdefault_cluster_id).result_of(common::xarchive_group_id);
    for (auto nodes : nodes_info) {
        xelection_info_t new_election_info{};
        new_election_info.consensus_public_key = nodes.pub_key;
        new_election_info.stake = nodes.stake;
        new_election_info.joined_version = common::xelection_round_t{0};
        new_election_info.genesis = true;
        new_election_info.miner_type = common::xminer_type_t::advance | common::xminer_type_t::validator | common::xminer_type_t::edge;

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(nodes.node_id);
        election_info_bundle.election_info(std::move(new_election_info));

        election_group_result.insert(std::move(election_info_bundle));
    }
    election_group_result.election_committee_version(common::xelection_round_t{0});
    election_group_result.timestamp(current_time);
    election_group_result.start_time(current_time);
    if (election_group_result.group_version().empty()) {
        election_group_result.group_version(common::xelection_round_t::max());
    }
    xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
}
#endif

void xtop_rec_elect_archive_contract::setup() {
    data::election::v0::xelection_result_store_t election_result_store;
    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        STRING_CREATE(property);
        serialization::xmsgpack_t<data::election::v0::xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
    }
}

void xtop_rec_elect_archive_contract::on_timer(const uint64_t current_time) {
#ifdef STATIC_CONSENSUS
    if (xstatic_election_center::instance().if_allow_elect()) {
        if (!executed_archive) {
            elect_config_nodes(current_time);
            return;
        }
#ifndef ELECT_WHEREAFTER
    return;
#endif
    } else {
        return;
    }
#endif

    XMETRICS_TIME_RECORD(XARCHIVE_ELECT "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XARCHIVE_ELECT "on_timer_cpu_time");
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xrec_elect_archive_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_rec_elect_archive_addr,
                     "xrec_elect_archive_contract_t instance is not triggled by sys_contract_rec_elect_archive_addr");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xrec_elect_archive_contract_t::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval) / 2 > TIME(), "xrec_elect_archive_contract_t::on_timer retried too many times");
    xinfo("xrec_elect_archive_contract_t::archive_elect %" PRIu64, current_time);

    auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
    if (chain_fork::xchain_fork_config_center_t::is_forked(fork_config.standalone_exchange_point, current_time)) {
        elect_archive(current_time);
    } else {
        elect_storage(current_time);
    }
}

// before fork point: standalone_exchange_point
void xtop_rec_elect_archive_contract::elect_storage(const uint64_t current_time){
    xrange_t<config::xgroup_size_t> archive_group_range{ 1, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size) };
    xrange_t<config::xgroup_size_t> exchange_group_range{0, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size)};

    auto standby_result_store =
        xvm::serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, sys_contract_rec_standby_pool_addr, data::XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_network_result = standby_result_store.result_of(network_id()).network_result();

    for (auto index = 0; index < XGET_CONFIG(legacy_archive_group_count); ++index) {
        top::common::xgroup_id_t archive_gid{ static_cast<top::common::xgroup_id_t::value_type>(common::xarchive_group_id_value_begin + index) };
        xkinfo("[xrec_elect_archive_contract_t] index: %d, archive_gid: %s, insert %s",
               index,
               archive_gid.to_string().c_str(),
               data::election::get_property_by_group_id(archive_gid).c_str());
        auto election_result_store =
            serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(archive_gid));

        auto & election_network_result = election_result_store.result_of(network_id());

        auto range = archive_group_range;
        if (archive_gid == common::xlegacy_exchange_group_id) {
            range = exchange_group_range;
        }

        if (elect_group(common::xstorage_zone_id,
                        common::xdefault_cluster_id,
                        archive_gid,
                        current_time,
                        current_time,
                        range,
                        standby_network_result,
                        election_network_result)) {
            xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, data::election::get_property_by_group_id(archive_gid), election_result_store);
        }
    }
}

void xtop_rec_elect_archive_contract::elect_archive(const uint64_t current_time) {
    xrange_t<config::xgroup_size_t> archive_group_range{1, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size)};

    auto standby_result_store =
        xvm::serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, sys_contract_rec_standby_pool_addr, data::XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_network_result = standby_result_store.result_of(network_id()).network_result();

    top::common::xgroup_id_t archive_gid = common::xarchive_group_id;
    xkinfo("[xrec_elect_archive_contract_t] archive_gid: %s, insert %s",
           archive_gid.to_string().c_str(),
           data::election::get_property_by_group_id(archive_gid).c_str());
    auto election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(archive_gid));

    auto & election_network_result = election_result_store.result_of(network_id());

    auto range = archive_group_range;

    if (elect_group(common::xstorage_zone_id, common::xdefault_cluster_id, archive_gid, current_time, current_time, range, standby_network_result, election_network_result)) {
        xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, data::election::get_property_by_group_id(archive_gid), election_result_store);
    }

    auto exchange_election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(common::xlegacy_exchange_group_id));
    bool clean_exchange_result{false};
    while (exchange_election_result_store.result_of(network_id()).size()) {
        exchange_election_result_store.erase(exchange_election_result_store.begin());
        clean_exchange_result = true;
    }
    if (clean_exchange_result) {
        xkinfo("[xrec_elect_archive_contract_t] clean old exchange election result.");
        xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
            *this, data::election::get_property_by_group_id(common::xlegacy_exchange_group_id), exchange_election_result_store);
    }
}

common::xnode_type_t xtop_rec_elect_archive_contract::standby_type(common::xzone_id_t const & zid,
                                                                   common::xcluster_id_t const & cid,
                                                                   common::xgroup_id_t const & gid) const {
    assert(!broadcast(zid));
    assert(!broadcast(cid));
    assert(!broadcast(gid));

    assert(zid == common::xstorage_zone_id);
    assert(cid == common::xdefault_cluster_id);
    assert(gid == common::xarchive_group_id || gid == common::xlegacy_exchange_group_id);

    if (gid == common::xarchive_group_id) {
        return common::xnode_type_t::storage_archive;
    }

    // todo remove after fork
    if (gid == common::xlegacy_exchange_group_id) {
        return common::xnode_type_t::storage_exchange;
    }

    assert(false);
    return common::xnode_type_t::invalid;
}

NS_END4
