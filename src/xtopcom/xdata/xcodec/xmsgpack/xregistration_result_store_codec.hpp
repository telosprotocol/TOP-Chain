// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
#include "xdata/xcodec/xmsgpack/xregistration_chain_result_codec.hpp"
#include "xdata/xregistration/xregistration_result_store.h"


NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xregistration_result_store_field_count{1};
XINLINE_CONSTEXPR std::size_t xregistration_result_store_index{0};

template <>
struct convert<top::data::registration::xregistration_result_store_t> final {
    msgpack::object const & operator()(msgpack::object const & o, top::data::registration::xregistration_result_store_t & result) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (o.via.array.size == 0) {
            return o;
        }

        switch (o.via.array.size - 1) {
        default: {
            XATTRIBUTE_FALLTHROUGH;
        }
        case xregistration_result_store_index: {
            result.results(o.via.array.ptr[xregistration_result_store_index].as<std::map<top::common::xnetwork_id_t, top::data::registration::xregistration_chain_result_t>>());
            XATTRIBUTE_FALLTHROUGH;
        }
        }

        return o;
    }
};

template <>
struct pack<::top::data::registration::xregistration_result_store_t> {
    template <typename StreamT>
    msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::data::registration::xregistration_result_store_t const & result) const {
        o.pack_array(xregistration_result_store_field_count);
        o.pack(result.results());
        return o;
    }
};

template <>
struct object_with_zone<::top::data::registration::xregistration_result_store_t> {
    void operator()(msgpack::object::with_zone & o, top::data::registration::xregistration_result_store_t const & result) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xregistration_result_store_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xregistration_result_store_index] = msgpack::object{result.results(), o.zone};
    }
};

NS_END1
}
NS_END1
