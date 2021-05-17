// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <stdint.h>
#include <string>

#include "xbasic/xoptional.hpp"
#include "xchain_upgrade_type.h"

namespace top {
    namespace chain_upgrade {

        extern xchain_fork_config_t mainnet_chain_config;
        extern xchain_fork_config_t testnet_chain_config;
        extern xchain_fork_config_t default_chain_config;

        /**
         * @brief chain fork config center
         *
         */
        class xtop_chain_fork_config_center {
        public:
            static xchain_fork_config_t const & chain_fork_config() noexcept;
            static bool is_forked(top::optional<xfork_point_t> const& fork_point, uint64_t target) noexcept;
        };
        using xchain_fork_config_center_t = xtop_chain_fork_config_center;
    }
}
