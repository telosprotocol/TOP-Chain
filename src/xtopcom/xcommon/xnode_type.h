// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <functional>
#include <string>

NS_BEG2(top, common)

/**
 * @brief Node type.  The type is devided into two parts, the real part and the virtual part.
 *        The lower half part is the real part and the higher half part is the virtual part.
 *        Real part the concrete type of the node.  Virtual part denotes the node logic location.
 */
enum class xenum_node_type : std::uint32_t {

    /// @brief Invalid node type.
    invalid = 0x00000000,

    ///@brief Network node.  Representing whole network.
    network = 0x01000000,

    /// @brief Zone node manages cluster nodes.
    zone = 0x02000000,

    /// @brief Cluster node manages group nodes
    cluster = 0x04000000,

    /// @brief Group node manages advance nodes & consensus nodes
    group = 0x08000000,

    /// @brief REC zone type or REC node type.
    rec = 0x00001000,
    committee = rec,

    /// @brief ZEC zone type or ZEC node type.
    zec = 0x00002000,

    /// @brief Archive zone type or archive node type.
    storage = 0x00004000,

    /// @brief Edge zone type or edge node type.
    edge = 0x00008000,

    /// @brief Frozen zone type or frozen node type. It is used for synchronizing data.
    frozen = 0x00000100,

    /// @brief Consensus Zone type.
    consensus = 0x00000200,

    /// @brief Auditor node.
    auditor = 0x00000001,

    /// @brief Validator node.
    validator = 0x00000002,

    archive = 0x00000004,
    full_node = 0x00000008,

    consensus_auditor = consensus | auditor,
    consensus_validator = consensus | validator,
    storage_archive = storage | archive,
    storage_full_node = storage | full_node,

    /// @brief all type
    all = 0x0000FFFF,
};
using xnode_type_t = xenum_node_type;

std::string to_string(xnode_type_t const type);

constexpr xnode_type_t operator&(xnode_type_t const lhs, xnode_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xnode_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xnode_type_t>::type>(rhs);
    return static_cast<xnode_type_t>(lhs_value & rhs_value);
#else
    return static_cast<xnode_type_t>(static_cast<std::underlying_type<xnode_type_t>::type>(lhs) & static_cast<std::underlying_type<xnode_type_t>::type>(rhs));
#endif
}

constexpr xnode_type_t operator|(xnode_type_t const lhs, xnode_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xnode_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xnode_type_t>::type>(rhs);
    return static_cast<xnode_type_t>(lhs_value | rhs_value);
#else
    return static_cast<xnode_type_t>(static_cast<std::underlying_type<xnode_type_t>::type>(lhs) | static_cast<std::underlying_type<xnode_type_t>::type>(rhs));
#endif
}

constexpr xnode_type_t operator~(xnode_type_t const value) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const raw_value = static_cast<std::underlying_type<xnode_type_t>::type>(value);
    return static_cast<xnode_type_t>(~raw_value);
#else
    return static_cast<xnode_type_t>(~static_cast<std::underlying_type<xnode_type_t>::type>(value));
#endif
}

xnode_type_t & operator&=(xnode_type_t & lhs, xnode_type_t const rhs) noexcept;

xnode_type_t & operator|=(xnode_type_t & lhs, xnode_type_t const rhs) noexcept;

xnode_type_t real_part_type(xnode_type_t const in) noexcept;

xnode_type_t virtual_part_type(xnode_type_t const in) noexcept;

template <xnode_type_t VNodeType>
constexpr bool has(xnode_type_t const type) noexcept {
    return VNodeType == (VNodeType & type);
}

bool
has(xnode_type_t const target, xnode_type_t const input) noexcept;

xnode_type_t reset_virtual_part(xnode_type_t const in, xnode_type_t const desired_virtual_part_type = xnode_type_t::invalid) noexcept;

xnode_type_t reset_real_part(xnode_type_t const in, xnode_type_t const desired_real_part_type = xnode_type_t::invalid) noexcept;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xnode_type_t> final {
    std::size_t operator()(top::common::xnode_type_t const type) const noexcept;
};

NS_END1
