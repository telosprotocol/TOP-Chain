// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "metrics_handler/counter_handler.h"
#include "metrics_handler/flow_handler.h"
#include "metrics_handler/timer_handler.h"
#include "metrics_handler/xmetrics_packet_info.h"
#include "xbasic/xrunnable.h"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"
#include "xmetrics_event.h"
#include "xmetrics_unit.h"

#include <chrono>
#include <map>
#include <string>
#include <thread>
NS_BEG2(top, metrics)

enum E_SIMPLE_METRICS_TAG {
    e_simple_begin = 0,
    xdata_table_counter = e_simple_begin,
    xdata_lightunit_counter,
    xdata_fullunit_counter,
    xdata_table_ref_counter,
    xdata_lightunit_ref_counter,
    xdata_fullunit_ref_counter,
    blockstore_cache_block_total,
    xdata_empty_block_counter,
    xdata_root_block_counter,
    xdata_empty_block_ref_counter,
    xdata_root_block_ref_counter,
    xdata_transaction_counter,
    xdata_transaction_ref_counter,
    xdata_xcons_transaction_t,
    xdata_receipt_t,
    xtxpool_xaccountobj_t,
    e_simple_total = xtxpool_xaccountobj_t + 1,
};

class e_metrics : public top::xbasic_runnable_t<e_metrics> {
public:
    XDECLARE_DELETED_COPY_AND_MOVE_SEMANTICS(e_metrics);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(e_metrics);

    static e_metrics & get_instance() {
        static e_metrics instance;
        return instance;
    }

    void start() override;
    void stop() override;

private:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(e_metrics);
    void run_process();
    void process_message_queue();
    void update_dump();
    void gauge_dump();

public:
    void timer_start(std::string metrics_name, time_point value);
    void timer_end(std::string metrics_name, time_point value, metrics_appendant_info info = 0, microseconds timed_out = DEFAULT_TIMED_OUT);
    void counter_increase(std::string metrics_name, int64_t value);
    void counter_decrease(std::string metrics_name, int64_t value);
    void counter_set(std::string metrics_name, int64_t value);
    void flow_count(std::string metrics_name, int64_t value, time_point timestamp);
    void gauge(E_SIMPLE_METRICS_TAG tag, int64_t value);

private:
    std::thread m_process_thread;
    std::size_t m_dump_interval;
    std::chrono::microseconds m_queue_procss_behind_sleep_time;
    handler::counter_handler_t m_counter_handler;
    handler::timer_handler_t m_timer_handler;
    handler::flow_handler_t m_flow_handler;
    constexpr static std::size_t message_queue_size{500000};
    top::threading::xthreadsafe_queue<event_message, std::vector<event_message>> m_message_queue{message_queue_size};
    std::map<std::string, metrics_variant_ptr> m_metrics_hub;  // {metrics_name, metrics_vaiant_ptr}
protected:
    struct simple_counter{
        std::atomic_long value;
        std::atomic_long call_count;
    };
    static simple_counter s_counters[e_simple_total]; // simple counter counter
    static metrics_variant_ptr s_metrics[e_simple_total]; // simple metrics dump info
};

class metrics_time_auto {
public:
    metrics_time_auto(std::string metrics_name, metrics_appendant_info key = std::string{"NOT SET appendant_info"}, microseconds timed_out = DEFAULT_TIMED_OUT)
      : m_metrics_name{metrics_name}, m_key{key}, m_timed_out{timed_out} {
        char c[15] = {0};
        snprintf(c, 14, "%p", (void*)(this));
        m_metrics_name = m_metrics_name + "&" + c;
        e_metrics::get_instance().timer_start(m_metrics_name, std::chrono::system_clock::now());
    }
    ~metrics_time_auto() { e_metrics::get_instance().timer_end(m_metrics_name, std::chrono::system_clock::now(), m_key, m_timed_out); }

    std::string m_metrics_name;
    metrics_appendant_info m_key;
    microseconds m_timed_out;
};

#ifdef ENABLE_METRICS
#define XMETRICS_INIT()                                                                                                                                                            \
    {                                                                                                                                                                              \
        auto & ins = top::metrics::e_metrics::get_instance();                                                                                                                      \
        ins.start();                                                                                                                                                               \
    }

#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream()  << x)).str()
#define ADD_THREAD_HASH(metrics_name) SSTR(metrics_name) + "&0x" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()))
#define STR_CONCAT_IMPL(a, b) a##b
#define STR_CONCAT(str_a, str_b) STR_CONCAT_IMPL(str_a, str_b)

#define MICROSECOND(timeout)                                                                                                                                                       \
    std::chrono::microseconds { timeout }

#define XMETRICS_TIME_RECORD(metrics_name)                                                                                                                                         \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name }
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)                                                                                                                                \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name, key }
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                             \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name, key, MICROSECOND(timeout) }

#define XMETRICS_TIMER_START(metrics_name) top::metrics::e_metrics::get_instance().timer_start(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP(metrics_name) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP_KEY(metrics_name, key) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key);

#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                              \
    top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key, MICROSECOND(timeout));

#define XMETRICS_COUNTER_INCREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_increase(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_decrease(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value) top::metrics::e_metrics::get_instance().counter_set(metrics_name, value)

#define XMETRICS_FLOW_COUNT(metrics_name, value) top::metrics::e_metrics::get_instance().flow_count(metrics_name, value, std::chrono::system_clock::now());

#define XMETRICS_PACKET_INFO(metrics_name, ...)                                                                                                                                    \
    top::metrics::handler::metrics_pack_unit STR_CONCAT(packet_info_auto_, __LINE__){metrics_name};                                                                                                       \
    top::metrics::handler::metrics_packet_impl(STR_CONCAT(packet_info_auto_, __LINE__), __VA_ARGS__);

#define XMETRICS_XBASE_DATA_CATEGORY_NEW(key) XMETRICS_COUNTER_INCREMENT("dataobject_cur_xbase_type" + std::to_string(key), 1);
#define XMETRICS_XBASE_DATA_CATEGORY_DELETE(key) XMETRICS_COUNTER_INCREMENT("dataobject_cur_xbase_type" + std::to_string(key), -1);
#define XMETRICS_GAUGE(TAG, value) top::metrics::e_metrics::get_instance().gauge(TAG, value)
#else
#define XMETRICS_INIT()
#define XMETRICS_TIME_RECORD(metrics_name)
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_TIMER_START(metrics_name)
#define XMETRICS_TIMER_STOP(metrics_name)
#define XMETRICS_TIMER_STOP_KEY(metrics_name, key)
#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_COUNTER_INCREMENT(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value)
#define XMETRICS_FLOW_COUNT(metrics_name, value)
#define XMETRICS_PACKET_INFO(metrics_name, ...)
#define XMETRICS_XBASE_DATA_CATEGORY_NEW(key)
#define XMETRICS_XBASE_DATA_CATEGORY_DELETE(key)
#define XMETRICS_GAUGE(TAG, value)
#endif

NS_END2
