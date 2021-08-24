// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/routing_utils.h"

#ifdef _MSC_VER
#    define _WINSOCKAPI_
#    include <windows.h>
#endif

#include "xbase/xhash.h"
#include "xkad/routing_table/local_node_info.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"

#include <algorithm>
#include <limits>
#include <locale>

namespace top {

namespace kadmlia {

void GetPublicEndpointsConfig(const top::base::Config & config, std::set<std::pair<std::string, uint16_t>> & boot_endpoints) {
    std::string public_endpoints;
    if (!config.Get("node", "public_endpoints", public_endpoints)) {
        TOP_INFO("get node.public_endpoints failed");
        return;
    }

    top::base::ParseEndpoints(public_endpoints, boot_endpoints);
}

bool CreateGlobalXid(const base::Config & config) try {
    assert(!global_node_id.empty());
    global_xid = base::GetRootKadmliaKey(global_node_id);
    return true;
} catch (...) {
    TOP_FATAL("catch ...");
    return false;
}

LocalNodeInfoPtr CreateLocalInfoFromConfig(const base::Config & config, base::KadmliaKeyPtr kad_key) try {
    bool first_node = false;
    config.Get("node", "first_node", first_node);
    std::string local_ip;
    if (!config.Get("node", "local_ip", local_ip)) {
        TOP_ERROR("get node local_ip from config failed!");
        return nullptr;
    }
    uint16_t local_port = 0;
    if (!config.Get("node", "local_port", local_port)) {
        TOP_ERROR("get node local_port from config failed!");
        return nullptr;
    }

    kadmlia::LocalNodeInfoPtr local_node_ptr = nullptr;
    local_node_ptr.reset(new top::kadmlia::LocalNodeInfo());
    if (!local_node_ptr->Init(local_ip, local_port, first_node, kad_key)) {
        TOP_ERROR("init local node info failed!");
        return nullptr;
    }

    uint16_t http_port = static_cast<uint16_t>(RandomUint32());
    config.Get("node", "http_port", http_port);
    uint16_t ws_port = static_cast<uint16_t>(RandomUint32());
    config.Get("node", "ws_port", ws_port);
    local_node_ptr->set_rpc_http_port(http_port);
    local_node_ptr->set_rpc_ws_port(ws_port);
    return local_node_ptr;
} catch (std::exception & e) {
    TOP_ERROR("catched error[%s]", e.what());
    return nullptr;
}

}  // namespace kadmlia

}  // namespace top
