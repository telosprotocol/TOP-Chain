﻿// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xblockmaker/xblockmaker_face.h"
#include "xvledger/xaccountindex.h"

NS_BEG2(top, blockmaker)

class xlighttable_builder_para_t : public xblock_builder_para_face_t {
 public:
    xlighttable_builder_para_t(const std::vector<xblock_ptr_t> & units, const xblockmaker_resources_ptr_t & resources)
    : xblock_builder_para_face_t(resources), m_batch_units(units) {}
    virtual ~xlighttable_builder_para_t() {}

    const std::vector<xblock_ptr_t> &               get_batch_units() const {return m_batch_units;}

 private:
    std::vector<xblock_ptr_t>                   m_batch_units;
};

class xfulltable_builder_para_t : public xblock_builder_para_face_t {
 public:
    xfulltable_builder_para_t(const data::xtable_mbt_ptr_t & last_full_index,
                              const data::xtable_mbt_binlog_ptr_t & latest_commit_index_binlog,
                              const std::vector<xblock_ptr_t> & uncommit_blocks,
                              const std::vector<xblock_ptr_t> & blocks_from_last_full,
                              const xblockmaker_resources_ptr_t & resources)
    : xblock_builder_para_face_t(resources), m_last_full_index(last_full_index),
    m_latest_commit_index_binlog(latest_commit_index_binlog), m_uncommit_blocks(uncommit_blocks), m_blocks_from_last_full(blocks_from_last_full) {}
    virtual ~xfulltable_builder_para_t() {}

    const data::xtable_mbt_ptr_t &          get_last_full_index() const {return m_last_full_index;}
    const data::xtable_mbt_binlog_ptr_t &   get_latest_commit_index_binlog() const {return m_latest_commit_index_binlog;}
    const std::vector<xblock_ptr_t> &       get_uncommit_blocks() const {return m_uncommit_blocks;}
    const std::vector<xblock_ptr_t> &       get_blocks_from_last_full() const {return m_blocks_from_last_full;}
 private:
    data::xtable_mbt_ptr_t          m_last_full_index;
    data::xtable_mbt_binlog_ptr_t   m_latest_commit_index_binlog;
    std::vector<xblock_ptr_t>       m_uncommit_blocks;
    std::vector<xblock_ptr_t>       m_blocks_from_last_full;
};

class xlighttable_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xaccount_ptr_t & prev_state,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para);
};

class xfulltable_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xaccount_ptr_t & prev_state,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para);

 protected:
    std::string                 make_block_statistics(const std::vector<xblock_ptr_t> & blocks);
};

NS_END2
