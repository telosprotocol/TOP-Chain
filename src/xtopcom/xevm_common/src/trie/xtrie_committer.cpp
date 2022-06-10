// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_committer.h"

#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG3(top, evm_common, trie)

std::pair<xtrie_hash_node_ptr_t, int32_t> xtop_trie_committer::Commit(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec) {
    if (db == nullptr) {
        ec = error::xerrc_t::trie_db_not_provided;
        return std::make_pair(nullptr, 0);
    }
    xtrie_node_face_ptr_t h;
    int32_t committed;
    std::tie(h, committed) = commit(n, db, ec);
    if (ec) {
        return std::make_pair(nullptr, 0);
    }
    xassert(h->type() == xtrie_node_type_t::hashnode);
    auto hashnode = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(h.get())));
    return std::make_pair(hashnode, committed);
}

std::pair<xtrie_node_face_ptr_t, int32_t> xtop_trie_committer::commit(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec) {
    // if this path is clean, use available cached data
    xtrie_hash_node_t hash;
    bool dirty;
    std::tie(hash, dirty) = n->cache();
    if (!hash.is_null() && !dirty) {
        return std::make_pair(std::make_shared<xtrie_hash_node_t>(hash), 0);
    }

    // Commit children, then parent, and remove remove the dirty flag.
    switch (n->type()) {
    case xtrie_node_type_t::shortnode: {
        auto cn = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(n.get())));
        // Commit child
        auto collapsed = cn->copy();

        // If the child is fullNode, recursively commit,
        // otherwise it can only be hashNode or valueNode.
        int32_t childCommitted{0};
        if (cn->Val->type() == xtrie_node_type_t::fullnode) {
            xtrie_node_face_ptr_t childV;
            int32_t committed;
            std::tie(childV, committed) = commit(cn->Val, db, ec);
            if (ec) {
                return std::make_pair(nullptr, 0);
            }
            collapsed->Val = childV;
            childCommitted = committed;
        }
        // The key needs to be copied, since we're delivering it to database
        collapsed->Key = hexToCompact(cn->Key);
        auto hashedNode = store(collapsed, db);
        if (hashedNode->type() == xtrie_node_type_t::hashnode) {
            auto hn = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(hashedNode.get())));
            return std::make_pair(hn, childCommitted + 1);
        }
        return std::make_pair(collapsed, childCommitted);
    }
    case xtrie_node_type_t::fullnode: {
        auto cn = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(n.get())));
        std::array<xtrie_node_face_ptr_t, 17> hashedKids;
        int32_t childCommitted{0};
        std::tie(hashedKids, childCommitted) = commitChildren(cn, db, ec);
        if (ec) {
            return std::make_pair(nullptr, 0);
        }

        auto collapsed = cn->copy();
        collapsed->Children = hashedKids;

        auto hashedNode = store(collapsed, db);
        if (hashedNode->type() == xtrie_node_type_t::hashnode) {
            auto hn = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(hashedNode.get())));
            return std::make_pair(hn, childCommitted + 1);
        }
        return std::make_pair(collapsed, childCommitted);
    }
    case xtrie_node_type_t::hashnode: {
        auto cn = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(n.get())));
        return std::make_pair(cn, 0);
    }
    default:
        // nil, valuenode shouldn't be committed
        xassert(false);
    }
    __builtin_unreachable();
}

std::pair<std::array<xtrie_node_face_ptr_t, 17>, int32_t> xtop_trie_committer::commitChildren(xtrie_full_node_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec) {
    std::array<xtrie_node_face_ptr_t, 17> children;
    int32_t committed{0};

    for (std::size_t index = 0; index < 16; ++index) {
        auto child = n->Children[index];
        if (child == nullptr)
            continue;

        // If it's the hashed child, save the hash value directly.
        // Note: it's impossible that the child in range [0, 15]
        // is a valueNode.
        if (child->type() == xtrie_node_type_t::hashnode) {
            auto hn = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(child.get())));
            children[index] = hn;
            continue;
        }

        // Commit the child recursively and store the "hashed" value.
        // Note the returned node can be some embedded nodes, so it's
        // possible the type is not hashNode.
        xtrie_node_face_ptr_t hashed;
        int32_t childCommitted;
        std::tie(hashed, childCommitted) = commit(child, db, ec);
        if (ec) {
            return std::make_pair(children, 0);
        }
        children[index] = hashed;
        committed += childCommitted;
    }

    // For the 17th child, it's possible the type is valuenode.
    if (n->Children[16] != nullptr) {
        children[16] = n->Children[16];
    }

    return std::make_pair(children, committed);
}

// store hashes the node n and if we have a storage layer specified, it writes
// the key/value pair to it and tracks any node->child references as well as any
// node->external trie references.
xtrie_node_face_ptr_t xtop_trie_committer::store(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db) {
    auto hash = n->cache().first;
    int32_t size{0};

    if (hash.is_null()) {
        // This was not generated - must be a small node stored in the parent.
        // In theory, we should apply the leafCall here if it's not nil(embedded
        // node usually contains value). But small value(less than 32bytes) is
        // not our target.
        return n;
    } else {
        // We have the hash already, estimate the RLP encoding-size of the node.
        // The size is used for mem tracking, does not need to be exact

        size = estimateSize(n);
    }
    if (false) {
        // todo leafCh
    } else {
        db->insert(xhash256_t{hash.data()}, size, n);
    }
    return std::make_shared<xtrie_hash_node_t>(hash);
}

int32_t xtop_trie_committer::estimateSize(xtrie_node_face_ptr_t n) {
    switch (n->type()) {
    case xtrie_node_type_t::shortnode: {
        auto shortnode = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(n.get())));
        return 3 + shortnode->Key.size() + estimateSize(shortnode->Val);
    }
    case xtrie_node_type_t::fullnode: {
        auto fullnode = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(n.get())));
        int32_t s = 3;
        for (std::size_t index = 0; index < 16; ++index) {
            auto child = fullnode->Children[index];
            if (child != nullptr) {
                s += estimateSize(child);
            } else {
                s++;
            }
        }
        return s;
    }
    case xtrie_node_type_t::valuenode: {
        auto valuenode = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(n.get())));
        return 1 + valuenode->data().size();
    }
    case xtrie_node_type_t::hashnode: {
        auto hashnode = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(n.get())));
        return 1 + hashnode->data().size();
    }
    default:
        xassert(false);
    }
    __builtin_unreachable();
}

NS_END3