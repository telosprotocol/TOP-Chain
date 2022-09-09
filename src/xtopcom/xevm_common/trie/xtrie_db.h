// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xevm_common/trie/xtrie_kv_db_face.h"
#include "xevm_common/trie/xtrie_node_fwd.h"

#include <functional>
#include <map>

NS_BEG3(top, evm_common, trie)

class xtop_trie_db_config {
public:
    xtop_trie_db_config() {
    }

    uint64_t Cache_size{0};   // Memory allowance (MB) to use for caching trie nodes in memory
    std::string Journal{""};  // Journal of clean cache to survive node restarts
    bool Preimages{true};     // Flag whether the preimage of trie key is recorded
};
using xtrie_db_config_t = xtop_trie_db_config;
using xtrie_db_config_ptr_t = std::shared_ptr<xtrie_db_config_t>;

// fwd:
class xtop_trie_cache_node;
using xtrie_cache_node_t = xtop_trie_cache_node;

class xtop_trie_db {
private:
    friend class xtop_trie_cache_node;
    xkv_db_face_ptr_t diskdb;  // Persistent storage for matured trie nodes

    std::map<xhash256_t, xbytes_t> cleans;
    std::map<xhash256_t, xtrie_cache_node_t> dirties;

    xhash256_t oldest;
    xhash256_t newest;

    std::map<xhash256_t, xbytes_t> preimages;  // Preimages of nodes from the secure trie

public:
    xtop_trie_db(xkv_db_face_ptr_t _diskdb) : diskdb(_diskdb) {
    }

public:
    // NewDatabase creates a new trie database to store ephemeral trie content before
    // its written out to disk or garbage collected. No read cache is created, so all
    // data retrievals will hit the underlying disk database.
    static std::shared_ptr<xtop_trie_db> NewDatabase(xkv_db_face_ptr_t diskdb);

    // NewDatabaseWithConfig creates a new trie database to store ephemeral trie content
    // before its written out to disk or garbage collected. It also acts as a read cache
    // for nodes loaded from disk.
    static std::shared_ptr<xtop_trie_db> NewDatabaseWithConfig(xkv_db_face_ptr_t diskdb, xtrie_db_config_ptr_t config);

public:
    xkv_db_face_ptr_t DiskDB() {
        return diskdb;
    }

public:
    // insert inserts a collapsed trie node into the memory database.
    // The blob size must be specified to allow proper size tracking.
    // All nodes inserted by this function will be reference tracked
    // and in theory should only used for **trie nodes** insertion.
    void insert(xhash256_t hash, int32_t size, xtrie_node_face_ptr_t node);

    // insertPreimage writes a new trie node pre-image to the memory database if it's
    // yet unknown. The method will NOT make a copy of the slice,
    // only use if the preimage will NOT be changed later on.
    //
    // Note, this method assumes that the database's lock is held!
    void insertPreimage(xhash256_t hash, xbytes_t const & preimage);

    // node retrieves a cached trie node from memory, or returns nil if none can be
    // found in the memory cache.
    xtrie_node_face_ptr_t node(xhash256_t hash);

    // Node retrieves an encoded cached trie node from memory. If it cannot be found
    // cached, the method queries the persistent database for the content.
    xbytes_t Node(xhash256_t hash, std::error_code & ec);

    xbytes_t preimage(xhash256_t hash) const;

    using AfterCommitCallback = std::function<void(xhash256_t const &)>;

    // Commit iterates over all the children of a particular node, writes them out
    // to disk, forcefully tearing down all references in both directions. As a side
    // effect, all pre-images accumulated up to this point are also written.
    //
    // Note, this method is a non-synchronized mutator. It is unsafe to call this
    // concurrently with other mutators.
    void Commit(xhash256_t hash, AfterCommitCallback cb, std::error_code & ec);

private:
    // commit is the private locked version of Commit.
    // void commit(xhash256_t hash, AfterCommitCallback cb, std::error_code & ec);

    xbytes_t preimageKey(xhash256_t hash_key) const;
};
using xtrie_db_t = xtop_trie_db;
using xtrie_db_ptr_t = std::shared_ptr<xtrie_db_t>;

class xtop_trie_cache_node {
private:
    friend class xtop_trie_db;

    xtrie_node_face_ptr_t node;  // Cached collapsed trie node, or raw rlp data
    uint16_t size;               // Byte size of the useful cached data

    uint32_t parents;                         // Number of live nodes referencing this one
    std::map<xhash256_t, uint16_t> children;  // External children referenced by this node

    xhash256_t flushPrev;  // Previous node in the flush-list
    xhash256_t flushNext;  // Next node in the flush-list

private:
    xtop_trie_cache_node(xtrie_node_face_ptr_t _node, uint16_t _size, xhash256_t _flushPrev) : node{_node}, size{_size}, flushPrev{_flushPrev} {
    }

private:
    // rlp returns the raw rlp encoded blob of the cached trie node, either directly
    // from the cache, or by regenerating it from the collapsed node.
    xbytes_t rlp();

    // obj returns the decoded and expanded trie node, either directly from the cache,
    // or by regenerating it from the rlp encoded blob.
    xtrie_node_face_ptr_t obj(xhash256_t hash);

    using onChildFunc = std::function<void(xhash256_t const &)>;

    // forChilds invokes the callback for all the tracked children of this node,
    // both the implicit ones from inside the node as well as the explicit ones
    // from outside the node.
    void forChilds(onChildFunc f);

    // forGatherChildren traverses the node hierarchy of a collapsed storage node and
    // invokes the callback for all the hashnode children.
    void forGatherChildren(xtrie_node_face_ptr_t n, onChildFunc f);
};
using xtrie_cache_node_t = xtop_trie_cache_node;

// simplifyNode traverses the hierarchy of an expanded memory node and discards
// all the internal caches, returning a node that only contains the raw data.
xtrie_node_face_ptr_t simplifyNode(xtrie_node_face_ptr_t n);

// expandNode traverses the node hierarchy of a collapsed storage node and converts
// all fields and keys into expanded memory form.
xtrie_node_face_ptr_t expandNode(xtrie_hash_node_t hash, xtrie_node_face_ptr_t n);

NS_END3
