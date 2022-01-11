/* http/EventTransferManager.h
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

#ifndef AFV_NATIVE_EVENTTRANSFERMANAGER_H
#define AFV_NATIVE_EVENTTRANSFERMANAGER_H

#include <unordered_set>
#include <curl/curl.h>
#include <event2/event.h>

#include "afv-native/http/TransferManager.h"

namespace afv_native {
    namespace http {
        /**
         * EventTransferManager is a TransferManager that relies on libevent to
         * notify it of elapsed events.
         */
        class EventTransferManager: public http::TransferManager {
        private:
            struct SocketInfo {
                struct event *ev;

                SocketInfo(): ev(nullptr)
                {}
            };

            struct event *mTimerEvent;

            static void evSocketCallback(evutil_socket_t fd, short events, void *arg);

            static void evTimerCallback(evutil_socket_t fd, short events, void *arg);

            int socketCallback(CURL *easy, curl_socket_t s, int what, void *socketp);

            static int curlSocketCallback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp);

            int timerCallback(CURLM *multi, long timeout_ms);

            static int curlTimerCallback(CURLM *multi, long timeout_ms, void *userp);

            std::unordered_set<SocketInfo *> mWatchedSockets;

        protected:
            struct event_base *mEvBase;

        public:
            explicit EventTransferManager(struct event_base *evBase);

            ~EventTransferManager() override;

            /** Process any outstanding events without blocking.
             *
             * This is actually a no-op with ETM as all the processing is driven
             * by libevent instead.
             */
            void process() override;
        };
    }
}

#endif //AFV_NATIVE_EVENTTRANSFERMANAGER_H
