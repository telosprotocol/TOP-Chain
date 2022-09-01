// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_sync.h"

#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/xerror/xerror.h"

#include <assert.h>

NS_BEG3(top, evm_common, trie)

// maxFetchesPerDepth is the maximum number of pending trie nodes per depth. The
// role of this value is to limit the number of trie nodes that get expanded in
// memory if the node was configured with a significant number of peers.
static const std::size_t maxFetchesPerDepth = 16384;

SyncPath newSyncPath(xbytes_t const & path) {
    SyncPath res;
    if (path.size() < 64) {
        res.push_back(hexToCompact(path));
    } else {
        res.push_back(hexToKeybytes(xbytes_t{path.begin(), path.begin() + 64}));
        res.push_back(hexToCompact(xbytes_t{path.begin() + 64, path.end()}));
    }
    return res;
}

Sync::Sync(xhash256_t const & root, xkv_db_face_ptr_t _database, LeafCallback callback) : database{_database} {
    AddSubTrie(root, xbytes_t{}, xhash256_t{}, callback);
}

std::shared_ptr<Sync> Sync::NewSync(xhash256_t const & root, xkv_db_face_ptr_t _database, LeafCallback callback) {
    return std::make_shared<Sync>(root, _database, callback);
}

Sync::~Sync() {
    for (auto _p : nodeReqs) {
        if (_p.second != nullptr) {
            free(_p.second);
        }
    }
    nodeReqs.clear();
    for (auto _p : codeReqs) {
        if (_p.second != nullptr) {
            free(_p.second);
        }
    }
    codeReqs.clear();
}

void Sync::AddSubTrie(xhash256_t const & root, xbytes_t const & path, xhash256_t const & parent, LeafCallback callback) {
    if (root == emptyRoot) {
        return;
    }

    if (membatch.hasNode(root)) {
        return;
    }

    assert(database != nullptr);
    std::error_code ec;

    if (HasTrieNode(database, root)) {
        return;
    }

    auto req = new request(path, root, callback);
    if (parent != xhash256_t{}) {
        auto ancestor = nodeReqs[parent];
        if (ancestor == nullptr) {
            xerror("sub-trie ancestor not found %s", parent.as_hex_str().c_str());
            return;
        }
        ancestor->deps++;
        req->parents.push_back(ancestor);
    }
    schedule(req);
}

void Sync::AddCodeEntry(xhash256_t const & hash, xbytes_t const & path, xhash256_t const & parent) {
    if (hash == emptyRoot) {
        return;
    }

    if (membatch.hasNode(hash)) {
        return;
    }
    assert(database != nullptr);
    std::error_code ec;
    if (HasCodeWithPrefix(database, hash)) {
        return;
    }

    auto req = new request(path, hash, true);
    if (parent != xhash256_t{}) {
        auto ancestor = nodeReqs[parent];
        if (ancestor == nullptr) {
            xerror("raw-entry ancestor not found %s", parent.as_hex_str().c_str());
        }
        ancestor->deps++;
        req->parents.push_back(ancestor);
    }
    schedule(req);
}

std::tuple<std::vector<xhash256_t>, SyncPath, std::vector<xhash256_t>> Sync::Missing(std::size_t max) {
    std::vector<xhash256_t> nodeHashes;
    SyncPath nodePaths;
    std::vector<xhash256_t> codeHashes;

    while (!queue.empty() && (max == 0 || nodeHashes.size() + codeHashes.size() < max)) {
        xhash256_t hash;
        int64_t prio;
        std::tie(hash, prio) = queue.top();

        // If we have too many already-pending tasks for this depth, throttle
        auto depth = (std::size_t)(prio >> 56);
        if (fetches[depth] > maxFetchesPerDepth) {
            break;
        }

        queue.pop();
        fetches[depth]++;

        if (nodeReqs.find(hash) != nodeReqs.end()) {
            nodeHashes.push_back(hash);
            auto new_path = newSyncPath(nodeReqs[hash]->path);
            nodePaths.insert(nodePaths.end(), new_path.begin(), new_path.end());
        } else {
            codeHashes.push_back(hash);
        }
    }

    return std::make_tuple(nodeHashes, nodePaths, codeHashes);
}

void Sync::Process(SyncResult const & result, std::error_code & ec) {
    // If the item was not requested either for code or node, bail out
    if (nodeReqs.find(result.Hash) == nodeReqs.end() && codeReqs.find(result.Hash) == codeReqs.end()) {
        ec = error::xerrc_t::trie_sync_not_requested;
        return;
    }

    bool filled{false};
    // There is an pending code request for this data, commit directly
    if (codeReqs.find(result.Hash) != codeReqs.end()) {
        auto req = codeReqs.at(result.Hash);
        if (req->data.empty()) {
            filled = true;
            req->data = result.Data;
            commit(req, ec);
        }
    }

    // There is an pending node request for this data, fill it.
    if (nodeReqs.find(result.Hash) != nodeReqs.end()) {
        auto req = nodeReqs.at(result.Hash);
        filled = true;
        // Decode the node data content and update the request
        auto node = xtrie_node_rlp::decodeNode(result.Hash, result.Data, ec);
        if (ec) {
            return;
        }
        req->data = result.Data;

        // Create and schedule a request for all the children nodes

        auto requests = children(req, node, ec);
        if (ec) {
            return;
        }

        if (requests.size() == 0 && req->deps == 0) {
            commit(req, ec);  // todo commit parameter ec has no error occasion.
        } else {
            req->deps += requests.size();
            for (auto child : requests) {
                schedule(child);
            }
        }
    }

    if (!filled) {
        ec = error::xerrc_t::trie_sync_already_processed;
        return;
    }
}

void Sync::Commit(xkv_db_face_ptr_t db) {
    for (auto const & p : membatch.nodes) {
        WriteTrieNode(db, p.first, p.second);
    }
    for (auto const & p : membatch.codes) {
        WriteCode(db, p.first, p.second);
    }
    membatch.clear();
    return;
}

std::size_t Sync::Pending() const {
    return nodeReqs.size() + codeReqs.size();
}

void Sync::schedule(request * req) {
    if (req->code) {
        if (codeReqs.find(req->hash) != codeReqs.end()) {
            codeReqs[req->hash]->parents.insert(codeReqs[req->hash]->parents.begin(), req->parents.begin(), req->parents.end());
            return;
        }
        codeReqs.insert(std::make_pair(req->hash, req));
    } else {
        if (nodeReqs.find(req->hash) != nodeReqs.end()) {
            nodeReqs[req->hash]->parents.insert(nodeReqs[req->hash]->parents.begin(), req->parents.begin(), req->parents.end());
            return;
        }
        nodeReqs.insert(std::make_pair(req->hash, req));
    }

    // Schedule the request for future retrieval. This queue is shared
    // by both node requests and code requests. It can happen that there
    // is a trie node and code has same hash. In this case two elements
    // with same hash and same or different depth will be pushed. But it's
    // ok the worst case is the second response will be treated as duplicated.
    assert(req->path.size() < 128);  // depth >= 128 will never happen, storage leaves will be included in their parents
    int64_t prio = (int64_t)req->path.size() << 56;
    for (std::size_t i = 0; i < 14 && i < req->path.size(); ++i) {
        prio |= (int64_t)(15 - req->path[i]) << (52 - i * 4);  // 15-nibble => lexicographic order
    }
    queue.push(req->hash, prio);
}

std::vector<Sync::request *> Sync::children(request * req, xtrie_node_face_ptr_t object, std::error_code & ec) {
    // Gather all the children of the node, irrelevant whether known or not

    // <path, node>
    std::vector<std::pair<xbytes_t, xtrie_node_face_ptr_t>> children;
    switch (object->type()) {
    case xtrie_node_type_t::shortnode: {
        auto node = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(object.get())));
        auto key = node->Key;
        if (hasTerm(key)) {
            key = xbytes_t{key.begin(), key.end() - 1};
        }
        xbytes_t combined_path;
        combined_path.insert(combined_path.end(), req->path.begin(), req->path.end());
        combined_path.insert(combined_path.end(), key.begin(), key.end());
        children.push_back(std::make_pair(combined_path, node->Val));
    }
    case xtrie_node_type_t::fullnode: {
        auto node = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(object.get())));
        for (std::size_t i = 0; i < 17; ++i) {
            if (node->Children[i] != nullptr) {
                auto child = node->Children[i];
                xbytes_t combined_path;
                combined_path.insert(combined_path.end(), req->path.begin(), req->path.end());
                combined_path.insert(combined_path.end(), xbyte_t(i));
                children.push_back(std::make_pair(combined_path, child));
            }
        }
    }
    default: {
        xerror("unknown node type :%zu", static_cast<std::size_t>(object->type()));
    }
    }

    // Iterate over the children, and request all unknown ones
    std::vector<request *> requests;
    requests.reserve(children.size());
    for (auto const & child_p : children) {
        // Notify any external watcher of a new key/value node
        if (req->callback != nullptr) {
            if (child_p.second->type() == xtrie_node_type_t::valuenode) {
                auto node = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(child_p.second.get())));
                std::vector<xbytes_t> paths;
                if (child_p.first.size() == 2 * 32) {
                    paths.push_back(hexToKeybytes(child_p.first));
                } else if (child_p.first.size() == 4 * 32) {
                    paths.push_back(hexToKeybytes(xbytes_t{child_p.first.begin(), child_p.first.begin() + 2 * 32}));
                    paths.push_back(hexToKeybytes(xbytes_t{child_p.first.begin() + 2 * 32, child_p.first.end()}));
                }
                req->callback(paths, child_p.first, node->data(), req->hash, ec);
                if (ec) {
                    return {};
                }
            }
        }

        // If the child references another node, resolve or schedule
        if (child_p.second->type() == xtrie_node_type_t::hashnode) {
            // Try to resolve the node from the local database
            auto node = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(child_p.second.get())));
            auto hash = xhash256_t{node->data()};
            if (membatch.hasNode(hash)) {
                continue;
            }

            // If database says duplicate, then at least the trie node is present
            // and we hold the assumption that it's NOT legacy contract code.
            if (HasTrieNode(database, hash)) {
                continue;
            }

            auto new_req = new request(child_p.first, hash, req->callback);
            new_req->parents.push_back(req);
            requests.push_back(new_req);
        }
    }

    return requests;
}

void Sync::commit(request * req, std::error_code & ec) {
    // Write the node content to the membatch
    if (req->code) {
        membatch.codes[req->hash] = req->data;
        codeReqs.erase(req->hash);
        fetches[req->path.size()]--;
    } else {
        membatch.nodes[req->hash] = req->data;
        nodeReqs.erase(req->hash);
        fetches[req->path.size()]--;
    }

    // Check all parents for completion
    for (auto const & parent : req->parents) {
        parent->deps--;
        if (parent->deps == 0) {
            commit(parent, ec);
            if (ec) {
                return;
            }
        }
    }
    free(req);
    return;
}

NS_END3