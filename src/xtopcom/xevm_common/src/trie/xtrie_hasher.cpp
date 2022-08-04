// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_hasher.h"

#include "xbasic/xhex.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xutility/xhash.h"

NS_BEG3(top, evm_common, trie)

std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> xtop_trie_hasher::hash(xtrie_node_face_ptr_t node, bool force) {
    {
        auto _cached = node->cache();
        if (!_cached.first.is_null()) {
            return std::make_pair(std::make_shared<xtrie_hash_node_t>(_cached.first), node);
        }
    }
    switch (node->type()) {
    case xtrie_node_type_t::shortnode: {
        auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
        auto result = hashShortNodeChildren(n);
        auto hashed = shortnodeToHash(result.first, force);
        if (hashed->type() == xtrie_node_type_t::hashnode) {
            result.second->flags.hash = static_cast<xtrie_hash_node_t *>(hashed.get())->data();
        } else {
            result.second->flags.hash = {};
        }

        return std::make_pair(hashed, result.second);
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
        auto result = hashFullNodeChildren(n);
        auto hashed = fullnodeToHash(result.first, force);
        if (hashed->type() == xtrie_node_type_t::hashnode) {
            result.second->flags.hash = static_cast<xtrie_hash_node_t *>(hashed.get())->data();
        } else {
            result.second->flags.hash = {};
        }
        return std::make_pair(hashed, result.second);
    }
    case xtrie_node_type_t::valuenode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::hashnode: {
        return {node, node};
        break;
    }
    default:
        xassert(false);
    }
    __builtin_unreachable();
}

xtrie_hash_node_ptr_t xtop_trie_hasher::hashData(xbytes_t input) {
    xdbg("hashData:(%zu) %s ", input.size(), top::to_hex(input).c_str());
    xbytes_t hashbuf;
    utl::xkeccak256_t hasher;
    // hasher.reset(); // make hasher class member , than need this.
    hasher.update(input.data(), input.size());
    hasher.get_hash(hashbuf);
    xdbg(" -> hashed data:(%zu) %s", hashbuf.size(), top::to_hex(hashbuf).c_str());
    return std::make_shared<xtrie_hash_node_t>(hashbuf);
}

std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> xtop_trie_hasher::proofHash(xtrie_node_face_ptr_t node) {
    switch (node->type()) {
    case xtrie_node_type_t::shortnode: {
        auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
        xtrie_short_node_ptr_t sn;
        xtrie_short_node_ptr_t _;
        std::tie(sn, _) = hashShortNodeChildren(n);
        return std::make_pair(sn, shortnodeToHash(sn, false));
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
        xtrie_full_node_ptr_t fn;
        xtrie_full_node_ptr_t _;
        std::tie(fn, _) = hashFullNodeChildren(n);
        return std::make_pair(fn, fullnodeToHash(fn, false));
    }
    default:
        // Value and hash nodes don't have children so they're left as were
        return std::make_pair(node, node);
    }
    __builtin_unreachable();
}

std::pair<xtrie_short_node_ptr_t, xtrie_short_node_ptr_t> xtop_trie_hasher::hashShortNodeChildren(xtrie_short_node_ptr_t node) {
    auto collapsed = node->copy();
    auto cached = node->copy();

    collapsed->Key = hexToCompact(node->Key);

    if (node->Val->type() == xtrie_node_type_t::shortnode || node->Val->type() == xtrie_node_type_t::fullnode) {
        auto res = hash(node->Val, false);
        collapsed->Val = res.first;
        cached->Val = res.second;
    }

    return std::make_pair(collapsed, cached);
}

std::pair<xtrie_full_node_ptr_t, xtrie_full_node_ptr_t> xtop_trie_hasher::hashFullNodeChildren(xtrie_full_node_ptr_t node) {
    auto collapsed = node->copy();
    auto cached = node->copy();

    // todo impl
    // if h.parallel{}
    // ...
    // else{
    for (std::size_t index = 0; index < 16; ++index) {
        auto child = node->Children[index];
        if (child != nullptr) {
            auto res = hash(child, false);
            collapsed->Children[index] = res.first;
            cached->Children[index] = res.second;
        } else {
            collapsed->Children[index] = std::make_shared<xtrie_value_node_t>(nilValueNode);
        }
    }
    // }
    return std::make_pair(collapsed, cached);
}

xtrie_node_face_ptr_t xtop_trie_hasher::shortnodeToHash(xtrie_short_node_ptr_t node, bool force) {
    tmp.Reset();

    std::error_code ec;
    node->EncodeRLP(tmp.data(), ec);
    xdbg("[shortnodeToHash] %s  -> %s ", top::to_hex(node->Key).c_str(), top::to_hex(tmp.data()).c_str());
    xassert(!ec);

    if (tmp.len() < 32 && !force) {
        return node;  // Nodes smaller than 32 bytes are stored inside their parent
    }
    return hashData(tmp.data());
}

xtrie_node_face_ptr_t xtop_trie_hasher::fullnodeToHash(xtrie_full_node_ptr_t node, bool force) {
    tmp.Reset();

    std::error_code ec;
    node->EncodeRLP(tmp.data(), ec);
    xdbg("[fullnodeToHash] -> %s ", top::to_hex(tmp.data()).c_str());
    xassert(!ec);

    if (tmp.len() < 32 && !force) {
        return node;  // Nodes smaller than 32 bytes are stored inside their parent
    }
    return hashData(tmp.data());
}

NS_END3