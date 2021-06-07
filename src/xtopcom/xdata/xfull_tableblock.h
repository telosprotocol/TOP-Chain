// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include "xbase/xobject_ptr.h"
#include "xvledger/xdataobj_base.hpp"
#include "xdata/xblock_paras.h"
#include "xdata/xblock.h"
#include "xvledger/xaccountindex.h"
#include "xdata/xblock_statistics_data.h"

NS_BEG2(top, data)

class xfulltable_block_para_t {
 public:
    xfulltable_block_para_t(const std::string & snapshot_hash, const xstatistics_data_t & statistics_data);
    ~xfulltable_block_para_t() = default;

    const xstatistics_data_t &  get_block_statistics_data() const {return m_block_statistics_data;}
    const std::string &         get_snapshot_hash() const {return m_snapshot_hash;}

 private:
    xstatistics_data_t      m_block_statistics_data;
    std::string             m_snapshot_hash;
};

class xfulltable_output_entity_t final : public xventity_face_t<xfulltable_output_entity_t, xdata_type_fulltable_output_entity> {
 protected:
    static XINLINE_CONSTEXPR char const * PARA_OFFDATA_ROOT           = "0";
 public:
    xfulltable_output_entity_t() = default;
    explicit xfulltable_output_entity_t(const std::string & offdata_root);
 protected:
    ~xfulltable_output_entity_t() = default;
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
 private:
    xfulltable_output_entity_t & operator = (const xfulltable_output_entity_t & other);
 public:
    virtual const std::string query_value(const std::string & key) override {return std::string();}
 public:
    void            set_offdata_root(const std::string & root);
    std::string     get_offdata_root() const;
 private:
    std::map<std::string, std::string>  m_paras;
};

// tableindex block chain
// input: an array of newest units
// output: the root of bucket merkle tree of all unit accounts index
class xfull_tableblock_t : public xblock_t {
 public:
    static XINLINE_CONSTEXPR char const * RESOURCE_NODE_SIGN_STATISTICS     = "2";

 protected:
    enum { object_type_value = enum_xdata_type::enum_xdata_type_max - xdata_type_fulltable_block };
 public:
    xfull_tableblock_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output);
 protected:
    virtual ~xfull_tableblock_t();
 private:
    xfull_tableblock_t();
    xfull_tableblock_t(const xfull_tableblock_t &);
    xfull_tableblock_t & operator = (const xfull_tableblock_t &);
 public:
    static int32_t get_object_type() {return object_type_value;}
    static xobject_t *create_object(int type);
    void *query_interface(const int32_t _enum_xobject_type_) override;

 public:
    xstatistics_data_t get_table_statistics() const;

 public:  // override base block api
    std::string get_offdata_hash() const override;
};

NS_END2
