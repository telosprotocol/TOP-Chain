// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, mbus)

// <editor-fold defaultstate="collapsed" desc="event type timer">

class xevent_timer_t : public xbus_event_t {
public:

    xevent_timer_t(uint32_t ms_interval = 1000,
            bool _sync = false) :
    xbus_event_t(xevent_major_type_timer,
    0,
    to_listener,
    _sync),
    interval(ms_interval) {
    }

    uint32_t interval;

};

using xevent_timer_ptr_t = xobject_ptr_t<xevent_timer_t>;

class xevent_chain_timer_t : public xbus_event_t {
public:

    xevent_chain_timer_t(base::xvblock_t* _time_block) :
    xbus_event_t(xevent_major_type_chain_timer,
    0,
    to_listener,
    false),
    time_block(_time_block) {
        if(time_block != nullptr) {
            time_block->add_ref();
        }
    }

    virtual ~xevent_chain_timer_t() {
        if(time_block != nullptr) {
            time_block->release_ref();
        }
    }

    base::xvblock_t* time_block{};
};

using xevent_chain_timer_ptr_t = xobject_ptr_t<xevent_chain_timer_t>;

// </editor-fold>

NS_END2
