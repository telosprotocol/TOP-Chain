// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvtransact.h"

namespace top
{
    namespace base
    {
        xvtransact_t::xvtransact_t()
            :xdataunit_t((enum_xdata_type)enum_xobject_type_vtransact)
        {
        }

        xvtransact_t::~xvtransact_t()
        {
        }

        xvtransaction_store_t::xvtransaction_store_t()
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine) {

        }
        xvtransaction_store_t::~xvtransaction_store_t() {
            if (m_raw_tx != nullptr) {
                m_raw_tx->release_ref();
            }
        }

        int32_t xvtransaction_store_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint8_t has_tx = m_raw_tx != nullptr;
            stream.write_compact_var(has_tx);
            if (has_tx) {
                m_raw_tx->serialize_to(stream);
            }
            stream.write_compact_var(m_send_block_height);
            stream.write_compact_var(m_recv_block_height);
            stream.write_compact_var(m_confirm_block_height);
            return (stream.size() - begin_size);
        }

        int32_t xvtransaction_store_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint8_t has_tx;
            stream.read_compact_var(has_tx);
            if (has_tx) {
                m_raw_tx = xdataunit_t::read_from(stream);
                xassert(m_raw_tx != nullptr);
            }
            stream.read_compact_var(m_send_block_height);
            stream.read_compact_var(m_recv_block_height);
            stream.read_compact_var(m_confirm_block_height);
            return (begin_size - stream.size());
        }

        void xvtransaction_store_t::set_raw_tx(xdataunit_t* tx) {
            if (m_raw_tx == nullptr && tx != nullptr) {
                tx->add_ref();
                m_raw_tx = tx;
            }
        }

        void xvtransaction_store_t::set_send_block_info(const xvtxindex_ptr & txindex) {
//            xassert(txindex->get_block_height() > 0);
            xassert(m_send_block_height == 0 && m_send_block_hash.empty());
            m_send_addr = txindex->get_block_addr();
            m_send_block_height = txindex->get_block_height();
            m_send_block_hash = txindex->get_block_hash();
            m_is_self_tx = txindex->is_self_tx();
            //set_raw_tx(txindex->get_raw_tx());
        }
        void xvtransaction_store_t::set_recv_block_info(const xvtxindex_ptr & txindex) {
//            xassert(txindex->get_block_height() > 0);
            xassert(m_recv_block_height == 0 && m_recv_block_hash.empty());
            m_recv_addr = txindex->get_block_addr();
            m_recv_block_height = txindex->get_block_height();
            m_recv_block_hash = txindex->get_block_hash();
            m_is_self_tx = false;
            //set_raw_tx(txindex->get_raw_tx());
        }
        void xvtransaction_store_t::set_confirm_block_info(const xvtxindex_ptr & txindex) {
//            xassert(txindex->get_block_height() > 0);
            xassert(m_confirm_block_height == 0 && m_confirm_block_hash.empty());
            m_confirm_block_height = txindex->get_block_height();
            m_confirm_block_hash = txindex->get_block_hash();
            m_is_self_tx = false;
            //set_raw_tx(txindex->get_raw_tx());
        }
    };//end of namespace of base
};//end of namespace of top
