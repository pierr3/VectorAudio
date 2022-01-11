/* http/TransferManager.h
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

#ifndef AFV_NATIVE_TRANSFERMANAGER_H
#define AFV_NATIVE_TRANSFERMANAGER_H

#include <unordered_map>
#include <memory>
#include <curl/curl.h>

namespace afv_native {
    namespace http {

        class Request;

        /** TransferManager manages all of the running HTTP/HTTPS transfers, making sure
         * completion notifications get fired, etc - and generally keeping things
         * running without blocking the thread.  It also keeps SSL session and
         * cookie data to share between requests.
         */
        class TransferManager {
        protected:
            CURLM *mCurlMultiHandle;
            CURLSH *mCurlShareHandle;

            std::unordered_map<CURL *, Request *> mPendingTransfers;

            /** processPendingMultiEvents triggers a reconcilation of any outstanding
             * completion notifications from curl and notifies the request objects
             * that their transfers are finished.
             */
            void processPendingMultiEvents();

        public:
            TransferManager();

            /* no copy constructor - TransferManger must not be copied as it would break the internal states. */
            TransferManager(const TransferManager &cpysrc) = delete;

            /** Joins a Request to the shared state carried by this TransferManager.
             *
             * @param req the Request object to join to the shared state.
             */
            [[deprecated("Use the shareState method on Request instead")]]
            void AddToSession(Request *req) const;

            void registerForAsyncCallback(Request &req);
            void removeAsyncCallback(Request &req);

            /** Schedules a Request to be processed asynchronously by this
             * TransferManager.  Once you call this method on a Request object,
             * you must not invoke it's local doSync() method.
             *
             * @param req The Request object to process
             */
            [[deprecated("Use the doAsync method on Request instead")]]
            virtual void HandleRequest(Request *req);

            virtual ~TransferManager();

            /** Process any outstanding events without blocking. */
            virtual void process();

            /** Return the internal CURLM handle
             *
             * @note This is not guaranteed to be available in the future.
             *
             * @return the internal CURLM handle
             */
            CURLM *getCurlMultiHandle() const;
        };
    }
}
#endif //AFV_NATIVE_TRANSFERMANAGER_H
