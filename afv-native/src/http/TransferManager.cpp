/* http/TransferManager.cpp
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

#include "afv-native/http/TransferManager.h"

#include <curl/curl.h>

#include "afv-native/http/Request.h"

using namespace afv_native::http;

TransferManager::TransferManager():
    mPendingTransfers()
{
    mCurlMultiHandle = curl_multi_init();
    mCurlShareHandle = curl_share_init();
    // share everything.
    curl_share_setopt(mCurlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(mCurlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(mCurlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(mCurlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(mCurlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_PSL);
}

TransferManager::~TransferManager()
{
    curl_multi_cleanup(mCurlMultiHandle);
    curl_share_cleanup(mCurlShareHandle);
}

void
TransferManager::process()
{
    int running = 0;

    curl_multi_perform(mCurlMultiHandle, &running);

    processPendingMultiEvents();
}

void
TransferManager::processPendingMultiEvents()
{
    struct CURLMsg *cMsg = nullptr;
    Request * req;
    int msgs_queued = 0;

    while (nullptr != (cMsg = curl_multi_info_read(mCurlMultiHandle, &msgs_queued))) {
        //GAH.  STUPID STUPID CURL.  Never return pointers from stack or other transient memory.
        auto msgCopy = *cMsg;

        req = mPendingTransfers[msgCopy.easy_handle];
        switch (msgCopy.msg) {
        case CURLMSG_DONE:
            // remove the easy handle from our management
            curl_multi_remove_handle(mCurlMultiHandle, msgCopy.easy_handle);
            // remove the shared_ptr hold we've got on the request itself.
            mPendingTransfers.erase(msgCopy.easy_handle);
            // and notify the request object.
            if (msgCopy.data.result == CURLE_OK) {
                req->notifyTransferCompleted();
            } else {
                req->notifyTransferError();
            }
            break;
        default:
            break;
        }
    }
}

void
TransferManager::AddToSession(Request *req) const
{
    if (req) {
        curl_easy_setopt(req->getCurlHandle(), CURLOPT_SHARE, mCurlShareHandle);
    }
}

void
TransferManager::HandleRequest(Request *req)
{
    if (req) {
        auto curlHandle = req->getCurlHandle();
        mPendingTransfers[curlHandle] = req;
        curl_multi_add_handle(mCurlMultiHandle, curlHandle);
    }
}

CURLM *
TransferManager::getCurlMultiHandle() const
{
    return mCurlMultiHandle;
}

void TransferManager::registerForAsyncCallback(Request &req)
{
    auto curlHandle = req.getCurlHandle();
    if (curlHandle != nullptr) {
        mPendingTransfers[curlHandle] = &req;
    }
}

void TransferManager::removeAsyncCallback(Request &req)
{
    auto curlHandle = req.getCurlHandle();
    if (curlHandle != nullptr) {
        mPendingTransfers.erase(curlHandle);
    }
}
