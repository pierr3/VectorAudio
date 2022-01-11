/* event/EventTimer.cpp
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2019 Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include "afv-native/event/EventTimer.h"

using namespace afv_native::event;

void EventTimer::evCallback(evutil_socket_t fd, short events, void *arg)
{
    auto *eventObj = reinterpret_cast<EventTimer *>(arg);
    eventObj->triggered();
}

EventTimer::EventTimer(struct event_base *evBase)
{
    mEvent = event_new(evBase,
            -1, // fd
            0, // event flags
            EventTimer::evCallback,
            this);
}

EventTimer::~EventTimer()
{
    event_del(mEvent);
    event_free(mEvent);
}

bool EventTimer::pending()
{
    return event_pending(mEvent, EV_TIMEOUT, nullptr);
}

void EventTimer::enable(unsigned int delayMs)
{
    struct timeval timeout = {
            static_cast<int>(delayMs)/1000,
            static_cast<int>(delayMs%1000)*1000
    };
    event_add(mEvent, &timeout);
}

void EventTimer::disable()
{
    event_del(mEvent);
}
