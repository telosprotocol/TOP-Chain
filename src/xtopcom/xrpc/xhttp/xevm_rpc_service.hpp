// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <memory>
#include "xrpc/xrpc_define.h"
#include "xrpc/xtraffic_controller.h"
#include "xrpc/xrule_manager.h"
#include "xrpc/xedge/xedge_method_manager.hpp"
#include "xrpc/prerequest/xpre_request_handler_mgr.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xerror/xrpc_error_json.h"
#include "xrpc/prerequest/xpre_request_handler_server.h"
#include "xtxstore/xtxstore_face.h"
#include "xrpc/xjson_proc.h"
#include "xrpc/eth_rpc/eth_error_code.h"

NS_BEG2(top, xrpc)
#define CLEAN_TIME          60
#define TIME_OUT_SECONDS    10800

struct xevm_rpc_once_flag {
static std::once_flag  m_once_flag;
};

template<typename T>
class xevm_rpc_service : public xevm_rpc_once_flag{
    typedef typename T::conn_type conn_type;
public:
    xevm_rpc_service(shared_ptr<xrpc_edge_vhost> edge_vhost,
                 common::xip2_t xip2,
                 bool archive_flag = false,
                 observer_ptr<base::xvblockstore_t> block_store = nullptr,
                 observer_ptr<base::xvtxstore_t> txstore = nullptr,
                 observer_ptr<elect::ElectMain> elect_main = nullptr,
                 observer_ptr<top::election::cache::xdata_accessor_face_t> const & election_cache_data_accessor = nullptr);
    void execute(shared_ptr<conn_type>& conn, const std::string& content, const std::string & ip);
    void clean_token_timeout(long seconds) noexcept;
    void cancel_token_timeout() noexcept;
    void reset_edge_method_mgr(shared_ptr<xrpc_edge_vhost> edge_vhost, common::xip2_t xip2);
public:
    unique_ptr<T>                                   m_edge_method_mgr_ptr;
    shared_ptr<asio::io_service>                    m_io_service;
private:
    unique_ptr<xfilter_manager>                     m_rule_mgr_ptr;
    unique_ptr<xpre_request_handler_mgr>            m_pre_request_handler_mgr_ptr;
    unique_ptr<asio::steady_timer>                  m_timer;//clean token timeout
};

template <typename T>
xevm_rpc_service<T>::xevm_rpc_service(shared_ptr<xrpc_edge_vhost> edge_vhost,
                              common::xip2_t xip2,
                              bool archive_flag,
                              observer_ptr<base::xvblockstore_t> block_store,
                              observer_ptr<base::xvtxstore_t> txstore,
                              observer_ptr<elect::ElectMain> elect_main,
                              observer_ptr<top::election::cache::xdata_accessor_face_t> const & election_cache_data_accessor) {
    m_rule_mgr_ptr = top::make_unique<xfilter_manager>();
    m_io_service = std::make_shared<asio::io_service>();
    m_edge_method_mgr_ptr = top::make_unique<T>(edge_vhost, xip2, m_io_service, archive_flag, block_store, txstore, elect_main, election_cache_data_accessor);
    m_pre_request_handler_mgr_ptr = top::make_unique<xpre_request_handler_mgr>();
    call_once(m_once_flag, [this](){
        clean_token_timeout(CLEAN_TIME);
    });
}

template<typename T>
void xevm_rpc_service<T>::reset_edge_method_mgr(shared_ptr<xrpc_edge_vhost> edge_vhost, common::xip2_t xip2) {
    m_edge_method_mgr_ptr->reset_edge_method(edge_vhost, xip2);
}

template <typename T>
void xevm_rpc_service<T>::execute(shared_ptr<conn_type> & conn, const std::string & content, const std::string & ip) {
    xpre_request_data_t pre_request_data;
    try {

            if (content.size() <= 900) {
                xinfo_rpc("evm rpc request:%s", content.c_str());
            } else {
                uint32_t part_num = (content.size() + 899) / 900;
                uint32_t i = 0;
                for (; i < (part_num - 1); i++) {
                    xinfo_rpc("evm rpc request%u:%s", i, content.substr(i * 900, 900).c_str());
                }
                xinfo_rpc("evm rpc request%u:%s", i, content.substr(i * 900).c_str());
            }

            Json::Reader reader;
            top::xrpc::xjson_proc_t json_proc;
            // reader.
            if (!reader.parse(content, json_proc.m_request_json)) {
                Json::Value err;
                err["error"]["code"] = eth::enum_eth_rpc_invalid_request;
                err["error"]["message"] = "invalid json request";
                xdbg("rpc request err");
                Json::FastWriter       writer;
                m_edge_method_mgr_ptr->write_response(conn, writer.write(err));
                return;
            }
            pre_request_data.m_request_map.emplace(RPC_SEQUENCE_ID, json_proc.m_request_json["id"].asString());
            json_proc.m_request_json["id"];
            m_edge_method_mgr_ptr->do_method(conn, json_proc, ip);
    } catch (const xrpc_error & e) {
        Json::Value err;
        err["error"]["code"] = eth::enum_eth_rpc_invalid_request;
        err["error"]["message"] = "invalid json request";
        Json::FastWriter writer;
        m_edge_method_mgr_ptr->write_response(conn, writer.write(err));
        xwarn_rpc("xevm_rpc_service xrpc_error exception:%s", e.what());
    } catch (...) {
        Json::Value err;
        err["error"]["code"] = eth::enum_eth_rpc_invalid_request;
        err["error"]["message"] = "invalid json request";
        Json::FastWriter writer;
        m_edge_method_mgr_ptr->write_response(conn, writer.write(err));        
        xwarn_rpc("xevm_rpc_service exception");
    }
}

template<typename T>
void xevm_rpc_service<T>::clean_token_timeout(long seconds) noexcept
{
    if (seconds == 0)
    {
        m_timer = nullptr;
        return;
    }
    if (m_timer == nullptr)
    {
        m_timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(*m_io_service));
    }
    m_timer->expires_from_now(std::chrono::seconds(seconds));
    //auto self(this->shared_from_this());
    m_timer->async_wait([this](const error_code &ec) {
        if (!ec) {
            std::string         account_addr;
            edge_account_info   account_info;
            xinfo_rpc("account:%lx", &(pre_request_service<>::m_account_info_map));

            while(pre_request_service<>::m_account_info_map.back(account_addr, account_info)) {
                auto _now = steady_clock::now();
                auto _duration = std::chrono::duration_cast<std::chrono::seconds>(_now - account_info.m_record_time_point).count();
                if (_duration <= TIME_OUT_SECONDS) {
                    break;
                }
                pre_request_service<>::m_account_info_map.erase(account_addr);
            }
            clean_token_timeout(CLEAN_TIME);
        }
    });
}

template<typename T>
void xevm_rpc_service<T>::cancel_token_timeout() noexcept
{
    if (m_timer) {
        error_code ec;
        m_timer->cancel(ec);
    }
}

NS_END2
