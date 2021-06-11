// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelection/xcache/xcluster_element.h"

#include "xbasic/xmemory.hpp"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xutility.h"
#include "xcommon/xelection_result_keepalive_strategy.h"
#include "xcommon/xip.h"
#include "xelection/xcache/xzone_element.h"
#include "xelection/xdata_accessor_error.h"

#include <cinttypes>
#include <cstdint>

NS_BEG3(top, election, cache)

xtop_cluster_element::xtop_cluster_element(common::xcluster_id_t const & cluster_id, std::shared_ptr<xzone_element_t> const & zone_element)
  : xbase_t{zone_element->network_id(), zone_element->zone_id(), cluster_id}, m_zone_element{zone_element} {}

std::shared_ptr<xzone_element_t> xtop_cluster_element::zone_element() const noexcept {
    return m_zone_element.lock();
}

bool xtop_cluster_element::exist(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const {
    XLOCK(m_group_elements_mutex);
    return exist_with_lock_hold_outside(group_id, logic_time);
}

xgroup_update_result_t xtop_cluster_element::add_group_element(common::xgroup_id_t const & group_id,
                                                               common::xversion_t const &  version,
                                                               common::xlogic_time_t const timestamp,
                                                               common::xlogic_time_t const start_time,
                                                               std::uint16_t const sharding_size,
                                                               std::uint64_t const associated_election_blk_height,
                                                               std::error_code & ec) {
    assert(!ec);
    xgroup_update_result_t result;

    if (group_id.empty()) {
        ec = xdata_accessor_errc_t::group_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": adding empty group id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()));

        return result;
    }

    if (version.empty()) {
        ec = xdata_accessor_errc_t::group_version_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": adding group %" PRIu16 " but group version is empty",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id.value()));

        return result;
    }

    XLOCK(m_group_elements_mutex);
    auto & group_info_store = m_group_elements[group_id];
    auto const it = group_info_store.find(start_time);
    if (it != std::end(group_info_store)) {
        auto group_element = top::get<std::shared_ptr<xgroup_element_t>>(*it);
        // encounter an election result with newer version info.
        if (version != group_element->version()) {
            if (group_element->version().empty()) {
                result = add_group_element_with_lock_hold_outside(group_id, version, timestamp, start_time, sharding_size, associated_election_blk_height, group_info_store, ec);
                if (ec) {
                    xwarn("%s %s", ec.category().name(), ec.message().c_str());
                }
            } else {
                ec = xdata_accessor_errc_t::group_version_mismatch;

                xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": adding group %" PRIu16 " but sees different version. incoming version %" PRIu64
                      "; local cache version %" PRIu64,
                      ec.category().name(),
                      static_cast<std::uint32_t>(network_id().value()),
                      static_cast<std::uint16_t>(zone_id().value()),
                      static_cast<std::uint16_t>(cluster_id().value()),
                      static_cast<std::uint16_t>(group_id.value()),
                      static_cast<std::uint64_t>(version.value()),
                      static_cast<std::uint64_t>(group_element->version().value()));

                return result;
            }
        } else {
            assert(version == group_element->version());

            // if this node already exists, try to add it again to get the correct added node.
            // result.added must be the latest data this node knows.
            result = add_group_element_with_lock_hold_outside(group_id, version, timestamp, start_time, sharding_size, associated_election_blk_height, group_info_store, ec);
            if (ec) {
                xwarn("%s %s", ec.category().name(), ec.message().c_str());
            }
        }

        return result;
    }

    result = add_group_element_with_lock_hold_outside(group_id, version, timestamp, start_time, sharding_size, associated_election_blk_height, group_info_store, ec);
    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
    }
    return result;
}

xgroup_update_result_t xtop_cluster_element::add_group_element(common::xgroup_id_t const & group_id,
                                                               common::xversion_t const &  version,
                                                               common::xlogic_time_t const timestamp,
                                                               common::xlogic_time_t const start_time,
                                                               std::uint16_t const sharding_size,
                                                               std::uint64_t const associated_election_blk_height) {
    std::error_code ec;
    auto ret = add_group_element(group_id, version, timestamp, start_time, sharding_size, associated_election_blk_height, ec);
    top::error::throw_error(ec);
    return ret;
}

std::shared_ptr<xgroup_element_t> xtop_cluster_element::group_element_by_height(common::xgroup_id_t const & group_id,
                                                                                uint64_t const election_blk_height,
                                                                                std::error_code & ec) const {
    assert(!ec);
    if (group_id.empty() || common::broadcast(group_id)) {
        ec = xdata_accessor_errc_t::group_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": looking for an empty group id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()));

        return {};
    }

    XLOCK(m_group_elements_mutex);
    auto const it = m_group_elements.find(group_id);
    if (it == std::end(m_group_elements)) {
        ec = xdata_accessor_errc_t::group_not_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't have group %" PRIu16 " with election block height %" PRIu64,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id.value()),
              static_cast<std::uint64_t>(election_blk_height));

        return {};
    }

    auto const & group_info_store = top::get<xgroup_info_container_t>(*it);
    for (auto const & group_info : group_info_store) {
        assert(top::get<std::shared_ptr<xgroup_element_t>>(group_info));
        auto const & group_element = top::get<std::shared_ptr<xgroup_element_t>>(group_info);

        if (group_element->associated_blk_height() == election_blk_height) {
            return group_element;
        }

        xdbg("xcluster_element_t::group_element comparing associated election blk height(%" PRIu64 ") vs queried election blk height(%" PRIu64 ")", group_element->associated_blk_height(), election_blk_height);
    }

    ec = xdata_accessor_errc_t::group_not_exist;

    xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't have group %" PRIu16 " with election block height %" PRIu64,
            ec.category().name(),
            static_cast<std::uint32_t>(network_id().value()),
            static_cast<std::uint16_t>(zone_id().value()),
            static_cast<std::uint16_t>(cluster_id().value()),
            static_cast<std::uint16_t>(group_id.value()),
            static_cast<std::uint64_t>(election_blk_height));

    return {};
}

std::shared_ptr<xgroup_element_t> xtop_cluster_element::group_element_by_height(common::xgroup_id_t const & group_id, uint64_t const election_blk_height) const {
    std::error_code ec;

    auto ret = group_element_by_height(group_id, election_blk_height, ec);
    top::error::throw_error(ec);
    return ret;
}

std::shared_ptr<xgroup_element_t> xtop_cluster_element::group_element(common::xgroup_id_t const & group_id, common::xversion_t const & version, std::error_code & ec) const {
    assert(!ec);
    if (group_id.empty() || common::broadcast(group_id)) {
        ec = xdata_accessor_errc_t::group_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": looking for an empty group id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()));

        return {};
    }

    if (version.empty()) {
        ec = xdata_accessor_errc_t::group_version_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": looking for group %" PRIu16 " but with empty version",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id.value()));

        return {};
    }

    XLOCK(m_group_elements_mutex);
    auto const it = m_group_elements.find(group_id);
    if (it == std::end(m_group_elements)) {
        ec = xdata_accessor_errc_t::group_not_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't have group %" PRIu16 " with version %" PRIu64,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id.value()),
              static_cast<std::uint64_t>(version.value()));

        return {};
    }

    auto const & group_info_store = top::get<xgroup_info_container_t>(*it);
    for (auto const & group_info : group_info_store) {
        assert(top::get<std::shared_ptr<xgroup_element_t>>(group_info));
        auto const & group_element = top::get<std::shared_ptr<xgroup_element_t>>(group_info);

        if (group_element->version() == version) {
            return group_element;
        }
    }

    ec = xdata_accessor_errc_t::group_not_exist;

    xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't have group %" PRIu16 " with version %" PRIu64,
          ec.category().name(),
          static_cast<std::uint32_t>(network_id().value()),
          static_cast<std::uint16_t>(zone_id().value()),
          static_cast<std::uint16_t>(cluster_id().value()),
          static_cast<std::uint16_t>(group_id.value()),
          static_cast<std::uint64_t>(version.value()));

    return {};
}

std::shared_ptr<xgroup_element_t> xtop_cluster_element::group_element(common::xgroup_id_t const & group_id, common::xversion_t const & version) const {
    std::error_code ec;
    auto ret = group_element(group_id, version, ec);
    top::error::throw_error(ec);
    return ret;
}

std::shared_ptr<xgroup_element_t> xtop_cluster_element::group_element_by_logic_time(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time, std::error_code & ec) const {
    assert(!ec);
    if (group_id.empty() || common::broadcast(group_id)) {
        ec = xdata_accessor_errc_t::group_id_empty;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": looking for an empty group id",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()));

        return {};
    }

    XLOCK(m_group_elements_mutex);
    auto const it = m_group_elements.find(group_id);
    if (it == std::end(m_group_elements)) {
        ec = xdata_accessor_errc_t::group_not_exist;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't have group %" PRIu16 " at logic time %" PRIu64 " group not found",
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id.value()),
              static_cast<std::uint64_t>(logic_time));

        return {};
    }

    auto const & group_info_store = top::get<xgroup_info_container_t>(*it);
    for (auto const & group_info : group_info_store) {
        if (top::get<common::xlogic_time_t const>(group_info) <= logic_time) {
            assert(top::get<std::shared_ptr<xgroup_element_t>>(group_info));
            return top::get<std::shared_ptr<xgroup_element_t>>(group_info);
        }
    }

    ec = xdata_accessor_errc_t::group_not_exist;

    xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't have group %" PRIu16 " at logic time %" PRIu64,
          ec.category().name(),
          static_cast<std::uint32_t>(network_id().value()),
          static_cast<std::uint16_t>(zone_id().value()),
          static_cast<std::uint16_t>(cluster_id().value()),
          static_cast<std::uint16_t>(group_id.value()),
          static_cast<std::uint64_t>(logic_time));

    return {};
}

std::shared_ptr<xgroup_element_t> xtop_cluster_element::group_element_by_logic_time(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const {
    std::error_code ec;
    auto ret = group_element_by_logic_time(group_id, logic_time, ec);
    top::error::throw_error(ec);
    return ret;
}

common::xversion_t xtop_cluster_element::group_version(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time, std::error_code & ec) const {
    assert(!ec);
    auto const group_element = this->group_element_by_logic_time(group_id, logic_time, ec);
    if (!ec) {
        assert(group_element);
        return group_element->version();
    }

    if (ec) {
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
    }

    return {};
}

common::xversion_t xtop_cluster_element::group_version(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const {
    std::error_code ec;
    auto ret = this->group_version(group_id, logic_time, ec);
    top::error::throw_error(ec);
    return ret;
}

std::vector<std::shared_ptr<xgroup_element_t>> xtop_cluster_element::children(common::xnode_type_t const  child_type,
                                                                              common::xlogic_time_t const logic_time,
                                                                              std::error_code & ec) const {
    assert(!ec);
    std::vector<std::shared_ptr<xgroup_element_t>> ret;

    XLOCK(m_group_elements_mutex);
    for (auto const & group_element_store : m_group_elements) {
        auto const & group_info_store = top::get<xgroup_info_container_t>(group_element_store);
        for (auto const & group_info : group_info_store) {
            if (top::get<common::xlogic_time_t const>(group_info) > logic_time) {
                continue;
            }

            auto const & group_element = top::get<std::shared_ptr<xgroup_element_t>>(group_info);
            assert(group_element);

            if ((child_type & group_element->type()) == common::xnode_type_t::invalid) {
                continue;
            }

            ret.push_back(group_element);
            break;
        }
    }

    if (ret.empty()) {
        ec = xdata_accessor_errc_t::no_children;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 " fail to find child group with type %s at logic time %" PRIu64,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              common::to_string(child_type).c_str(),
              static_cast<std::uint64_t>(logic_time));
    }

    return ret;
}

std::vector<std::shared_ptr<xgroup_element_t>> xtop_cluster_element::children(common::xnode_type_t const child_type, common::xlogic_time_t const logic_time) const {
    std::error_code ec;
    auto ret = children(child_type, logic_time, ec);
    top::error::throw_error(ec);
    return ret;
}

bool xtop_cluster_element::exist_with_lock_hold_outside(common::xgroup_id_t const & group_id) const {
    return find_with_lock_hold_outside(group_id) != std::end(m_group_elements);
}

bool xtop_cluster_element::exist_with_lock_hold_outside(common::xgroup_id_t const & group_id, common::xlogic_time_t const logic_time) const {
    auto const iterator = find_with_lock_hold_outside(group_id);
    if (iterator == std::end(m_group_elements)) {
        return false;
    }

    auto const & group_info_container = top::get<xgroup_info_container_t>(*iterator);
    return group_info_container.find(logic_time) != std::end(group_info_container);
}

xtop_cluster_element::xgroup_info_container_t::const_iterator xtop_cluster_element::find_with_lock_hold_outside(common::xgroup_id_t const & group_id,
                                                                                                                common::xlogic_time_t const logic_time) const {
    auto const iterator = find_with_lock_hold_outside(group_id);
    if (iterator == std::end(m_group_elements)) {
        return xtop_cluster_element::xgroup_info_container_t::const_iterator{};
    }

    return top::get<xgroup_info_container_t>(*iterator).find(logic_time);
}

xtop_cluster_element::xgroup_elements_container_t::const_iterator xtop_cluster_element::find_with_lock_hold_outside(common::xgroup_id_t const & group_id) const {
    return m_group_elements.find(group_id);
}

xgroup_update_result_t xtop_cluster_element::add_group_element_with_lock_hold_outside(common::xgroup_id_t const & group_id,
                                                                                      common::xversion_t const & version,
                                                                                      common::xlogic_time_t const timestamp,
                                                                                      common::xlogic_time_t const start_time,
                                                                                      std::uint16_t const sharding_size,
                                                                                      std::uint64_t const associated_election_blk_height,
                                                                                      xgroup_info_container_t & group_info_container,
                                                                                      std::error_code & ec) {
    assert(!ec);
    xgroup_update_result_t result;
    std::shared_ptr<xgroup_element_t> group_element;

    auto const zone_element_type = common::node_type_from(zone_id());
    switch (zone_element_type) {
    case common::xnode_type_t::committee: {
        assert(zone_id() == common::xcommittee_zone_id);
        group_element = std::make_shared<xgroup_element_t>(version, group_id, sharding_size, associated_election_blk_height, shared_from_this());
        break;
    }

    case common::xnode_type_t::consensus: {
        auto const consensus_type = common::node_type_from(zone_id(), cluster_id(), group_id);
        if (consensus_type == common::xnode_type_t::invalid) {
            assert(false);
            ec = xdata_accessor_errc_t::group_type_mismatch;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't recgnize group %" PRIu16,
                  ec.category().name(),
                  static_cast<std::uint32_t>(network_id().value()),
                  static_cast<std::uint16_t>(zone_id().value()),
                  static_cast<std::uint16_t>(cluster_id().value()),
                  static_cast<std::uint16_t>(group_id.value()));

            return {};
        }

        group_element = std::make_shared<xgroup_element_t>(version, group_id, sharding_size, associated_election_blk_height, shared_from_this());
        break;
    }

    case common::xnode_type_t::storage: {
        assert(zone_id() == common::xarchive_zone_id);

        auto const archive_type = common::node_type_from(zone_id(), cluster_id(), group_id);
        if (archive_type == common::xnode_type_t::invalid) {
            assert(false);
            ec = xdata_accessor_errc_t::group_type_mismatch;

            xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": doesn't recgnize group %" PRIu16,
                  ec.category().name(),
                  static_cast<std::uint32_t>(network_id().value()),
                  static_cast<std::uint16_t>(zone_id().value()),
                  static_cast<std::uint16_t>(cluster_id().value()),
                  static_cast<std::uint16_t>(group_id.value()));

            return {};
        }

        group_element = std::make_shared<xgroup_element_t>(version, group_id, sharding_size, associated_election_blk_height, shared_from_this());
        break;
    }

    case common::xnode_type_t::edge: {
        assert(zone_id() == common::xedge_zone_id);
        group_element = std::make_shared<xgroup_element_t>(version, group_id, sharding_size, associated_election_blk_height, shared_from_this());
        break;
    }

    case common::xnode_type_t::zec: {
        assert(zone_id() == common::xzec_zone_id);
        group_element = std::make_shared<xgroup_element_t>(version, group_id, sharding_size, associated_election_blk_height, shared_from_this());
        break;
    }

    case common::xnode_type_t::frozen: {
        assert(zone_id() == common::xfrozen_zone_id);
        group_element = std::make_shared<xgroup_element_t>(version, group_id, sharding_size, associated_election_blk_height, shared_from_this());
        break;
    }

    default: {
        assert(false);
        ec = xdata_accessor_errc_t::invalid_node_type;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": unknown zone id %" PRIu16,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(zone_id().value()));

        return {};
    }
    }

    group_element->rotation_status(common::xrotation_status_t::started, start_time);

    group_info_container.insert({start_time, group_element});

    result.added = top::get<std::shared_ptr<xgroup_element_t>>(*group_info_container.begin());
    auto const added_group_start_time = top::get<common::xlogic_time_t const>(*group_info_container.begin());

    if (result.added != group_element) {
        ec = xdata_accessor_errc_t::election_data_historical;

        xwarn("%s network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": added an historical group %" PRIu16 " at address %s start time %" PRIu64,
              ec.category().name(),
              static_cast<std::uint32_t>(network_id().value()),
              static_cast<std::uint16_t>(zone_id().value()),
              static_cast<std::uint16_t>(cluster_id().value()),
              static_cast<std::uint16_t>(group_id.value()),
              group_element->address().to_string().c_str(),
              start_time);
    }

    xwarn("[cluster_element] network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": added group %" PRIu16 " at address %s start time %" PRIu64
          " created group at address %s start time %" PRIu64,
          static_cast<std::uint32_t>(network_id().value()),
          static_cast<std::uint16_t>(zone_id().value()),
          static_cast<std::uint16_t>(cluster_id().value()),
          static_cast<std::uint16_t>(group_id.value()),
          result.added->address().to_string().c_str(),
          added_group_start_time,
          group_element->address().to_string().c_str(),
          start_time);

    auto const & keepalive_strategy = common::xdefault_election_result_keepalive_strategy;
    if (group_info_container.size() > keepalive_strategy.faded_threshold_count) {
        auto faded_it = std::next(std::begin(group_info_container), keepalive_strategy.faded_threshold_count);
        while (faded_it != std::end(group_info_container)) {
            auto & faded = top::get<std::shared_ptr<xgroup_element_t>>(*faded_it);
            if (faded->fade_time() == common::xjudgement_day) {
                faded->rotation_status(common::xrotation_status_t::faded, start_time);
                xwarn("[cluster_element] network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": fades group at address %s",
                      static_cast<std::uint32_t>(network_id().value()),
                      static_cast<std::uint16_t>(zone_id().value()),
                      static_cast<std::uint16_t>(cluster_id().value()),
                      faded->address().to_string().c_str());
            }

            if (result.faded == nullptr) {
                result.faded = faded;
            }

            ++faded_it;
        }
    }

    if (group_info_container.size() > keepalive_strategy.outdated_threshold_count) {
        auto outdated_it = std::next(std::begin(group_info_container), keepalive_strategy.outdated_threshold_count);
        while (outdated_it != std::end(group_info_container)) {
            auto outdated = top::get<std::shared_ptr<xgroup_element_t>>(*outdated_it);
            assert(outdated != nullptr);

            if (outdated->outdate_time() == common::xjudgement_day) {
                outdated->rotation_status(common::xrotation_status_t::outdated, start_time);
                xwarn("[cluster_element] network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": oudates group at address %s",
                      static_cast<std::uint32_t>(network_id().value()),
                      static_cast<std::uint16_t>(zone_id().value()),
                      static_cast<std::uint16_t>(cluster_id().value()),
                      outdated->address().to_string().c_str());
            }

            if (result.outdated == nullptr) {
                result.outdated = outdated;
            }

            if (outdated->rotation_status(timestamp) == common::xrotation_status_t::outdated) {
                outdated_it = group_info_container.erase(outdated_it);
                xwarn("[cluster_element] network %" PRIu32 " zone %" PRIu16 " cluster %" PRIu16 ": erases group at address %s",
                      static_cast<std::uint32_t>(network_id().value()),
                      static_cast<std::uint16_t>(zone_id().value()),
                      static_cast<std::uint16_t>(cluster_id().value()),
                      outdated->address().to_string().c_str());
            } else {
                ++outdated_it;
            }
        }
    }

    return result;
}

NS_END3
