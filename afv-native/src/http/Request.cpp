/* http/Request.cpp
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2015,2019-2020 Christopher Collins
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

#include "afv-native/http/Request.h"

#include <string>
#include <algorithm>
#include <cstring>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "afv-native/http/EventTransferManager.h"

using namespace afv_native::http;
using namespace std;
using json = nlohmann::json;

Request::Request(const string &url, Method method):
        mMethod(method),
        mURL(url),
        mFollowRedirect(method == Method::GET),
        mProgress(Progress::New),
        mCurlHandle(),
        mHeaders(nullptr),
        mTM(nullptr),
        mReq(),
        mReqBufOffset(0),
        mRespStatusCode(0),
        mRespContentType(),
        mResp(),
        mCurlErrorBuffer(),
        mCompletionCallback(),
        mDownloadTotal(0),
        mDownloadProgress(0),
        mUploadTotal(0),
        mUploadProgress(0)
{
    ::memset(mCurlErrorBuffer, 0, CURL_ERROR_SIZE);
}

Request::~Request()
{
    reset();
    curl_easy_cleanup(mCurlHandle);
    mCurlHandle = nullptr;

    if (mHeaders != nullptr) {
        curl_slist_free_all(mHeaders);
        mHeaders = nullptr;
    }
}

void Request::reset()
{
    if (mCurlHandle) {
        if (mTM != nullptr) {
            curl_multi_remove_handle(mTM->getCurlMultiHandle(), mCurlHandle);
            mTM->removeAsyncCallback(*this);
            mTM = nullptr;
        }
        curl_easy_cleanup(mCurlHandle);
        mCurlHandle = nullptr;
    }
    if (mHeaders != nullptr) {
        curl_slist_free_all(mHeaders);
        mHeaders = nullptr;
    }
    mResp.clear();
    mReq.clear();
    mReqBufOffset = 0;
    mProgress = Progress::New;
}

bool Request::setupHandle()
{
    assert(mCurlHandle == nullptr);
    mCurlHandle = curl_easy_init();
    curl_easy_setopt(mCurlHandle, CURLOPT_URL, mURL.c_str());
    curl_easy_setopt(mCurlHandle, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(mCurlHandle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(mCurlHandle, CURLOPT_ERRORBUFFER, mCurlErrorBuffer);
    curl_easy_setopt(mCurlHandle, CURLOPT_USERAGENT, "AFV-Native/1.0");
    curl_easy_setopt(mCurlHandle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(mCurlHandle, CURLOPT_XFERINFOFUNCTION, curlTransferInfoCallback);
    curl_easy_setopt(mCurlHandle, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(mCurlHandle, CURLOPT_READFUNCTION, curlReadCallback);
    curl_easy_setopt(mCurlHandle, CURLOPT_READDATA, this);
    if (mHeaders != nullptr) {
        curl_easy_setopt(mCurlHandle, CURLOPT_HTTPHEADER, mHeaders);
    }

    /* Disable Nagle because Mac says so.... */
    curl_easy_setopt(mCurlHandle, CURLOPT_TCP_NODELAY, 1);

    /*FIXME:  We do not verify peers (yet).  This is not a code issue, but a larger OpenSSL management one.
     *
     * We do not verify peers because libcurl is a pain in the ass and does not
     * use the system configured CAs in all cases.
     *
     * We do not verify peers because we do not want to have to hand adjust this
     * all the damned time.
     */
    curl_easy_setopt(mCurlHandle, CURLOPT_SSL_VERIFYPEER, 0);

    switch (mMethod) {
    case Method::GET:
        curl_easy_setopt(mCurlHandle, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, 1);
        break;
    case Method::POST:
        curl_easy_setopt(mCurlHandle, CURLOPT_POST, 1);
        curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, 0);
        curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDS, nullptr);
        curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDSIZE, mReq.size());
        break;
    case Method::PUT:
        curl_easy_setopt(mCurlHandle, CURLOPT_UPLOAD, 1);
        curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, 0);
        curl_easy_setopt(mCurlHandle, CURLOPT_INFILESIZE, mReq.size());
        break;
    case Method::DEL:
        curl_easy_setopt(mCurlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, 0);
        break;
    }
    curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, mFollowRedirect);
    return true;
}

void
Request::setFollowRedirect(bool follow)
{
    mFollowRedirect = follow;
}


void
Request::setHeader(const std::string &header, const std::string &value)
{
    struct curl_slist *newHeaders;
    if (value.empty()) {
        auto headerOut = header + ";";
        newHeaders = curl_slist_append(mHeaders, headerOut.c_str());
    } else {
        auto headerOut = header + ": " + value;
        newHeaders = curl_slist_append(mHeaders, headerOut.c_str());
    }
    if (newHeaders != nullptr) {
        mHeaders = newHeaders;
    }
}

void
Request::setRequestBody(const unsigned char *buf, size_t len)
{
    mReq.resize(len);
    ::memcpy(mReq.data(), buf, len);
}

void
Request::setRequestBody(const std::string &body)
{
    auto stringLen = body.length();
    mReq.resize(stringLen);
    ::memcpy((void *) mReq.data(), body.data(), stringLen);
}

void
Request::clearRequestBody()
{
    mReq.clear();
    mReqBufOffset = 0;
}

void
Request::setRequestBody(const nlohmann::json &j)
{
    setRequestBody(j.dump());
}

size_t
Request::curlReadCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    auto *r = reinterpret_cast<Request *>(userdata);
    return r->readCallback(buffer, size, nitems);
}

size_t
Request::readCallback(char *buffer, size_t size, size_t nitems)
{
    size_t reqBufLen = mReq.size();
    size_t totalRead = size * nitems;
    if ((size * nitems > 0) && (mReqBufOffset > reqBufLen)) {
        // attempted to read past the end of the buffer.  give up.
        return CURL_READFUNC_ABORT;
    }
    auto max_read = std::min<size_t>(reqBufLen - mReqBufOffset, totalRead);
    if (max_read > 0) {
        ::memcpy(buffer, mReq.data() + mReqBufOffset, max_read);
        mReqBufOffset += max_read;
    } else {
        // push the offset past the end to prevent
        mReqBufOffset = reqBufLen + 1;
    }
    return max_read;
}

size_t
Request::curlWriteCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    auto *r = reinterpret_cast<Request *>(userdata);
    return r->writeCallback(buffer, size, nitems);
}

size_t
Request::writeCallback(char *buffer, size_t size, size_t nitems)
{
    size_t offset = mResp.size();
    const size_t blockSize = size * nitems;
    mResp.resize(offset + blockSize, 0);
    ::memcpy(mResp.data() + offset, buffer, blockSize);
    return blockSize;
}

bool
Request::doSync()
{
    if (!setupHandle()) {
        return false;
    }
    mProgress = Progress::Connecting;
    memset(mCurlErrorBuffer, 0, CURL_ERROR_SIZE);
    auto rv = curl_easy_perform(mCurlHandle);
    if (rv == CURLE_OK) {
        notifyTransferCompleted();
        return true;
    } else {
        notifyTransferError();
        return false;
    }
}

void
Request::notifyTransferError()
{
    mProgress = Progress::Error;
    mTM = nullptr;
    // chain the callback if necessary.
    if (mCompletionCallback) {
        mCompletionCallback(this, false);
    }
}

std::string
Request::getCurlError() const
{
    return string(mCurlErrorBuffer);
}

Progress
Request::getProgress() const
{
    return mProgress;
}

int
Request::getStatusCode() const
{
    return mRespStatusCode;
}

std::string
Request::getContentType() const
{
    return mRespContentType;
}

int
Request::curlTransferInfoCallback(
        void *clientp,
        curl_off_t dltotal,
        curl_off_t dlnow,
        curl_off_t ultotal,
        curl_off_t ulnow)
{
    auto *r = reinterpret_cast<Request *>(clientp);
    r->transferInfoCallback(dltotal, dlnow, ultotal, ulnow);
    return 0;
}

void
Request::transferInfoCallback(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    mProgress = Progress::Transferring;
    mDownloadTotal = dltotal;
    mDownloadProgress = dlnow;
    mUploadTotal = ultotal;
    mUploadProgress = ulnow;
}

void
Request::notifyTransferCompleted()
{
    mProgress = Progress::Finished;
    long resp_code;
    if (CURLE_OK == curl_easy_getinfo(mCurlHandle, CURLINFO_RESPONSE_CODE, &resp_code)) {
        // downcast on Unixen. (sizeof(long) > sizeof(int)).
        mRespStatusCode = resp_code;
    }

    char *ct_ptr = nullptr;
    if (CURLE_OK == curl_easy_getinfo(mCurlHandle, CURLINFO_CONTENT_TYPE, &ct_ptr)) {
        if (ct_ptr != nullptr) {
            mRespContentType = string(ct_ptr);
        } else {
            mRespContentType = "";
        }
    }
    // As the transfer manager lets go of us after completion, disassociate from
    // the TransferManager.
    mTM = nullptr;

    // chain the callback if necessary.
    if (mCompletionCallback) {
        mCompletionCallback(this, true);
    }
}

int
Request::getDownloadTotal() const
{
    return mDownloadTotal;
}

int
Request::getDownloadProgress() const
{
    return mDownloadProgress;
}

int
Request::getUploadTotal() const
{
    return mUploadTotal;
}

int
Request::getUploadProgress() const
{
    return mUploadProgress;
}

string
Request::getResponseBody() const
{
    return string(reinterpret_cast<const char *>(mResp.data()), mResp.size());
}

CURL *
Request::getCurlHandle() const
{
    return mCurlHandle;
}

void Request::setCompletionCallback(std::function<void(Request *, bool)> cb)
{
    mCompletionCallback = cb;
}

bool Request::doAsync(TransferManager &transferManager)
{
    if (!setupHandle()) {
        return false;
    }

    auto curlMultiHandle = transferManager.getCurlMultiHandle();
    curl_multi_add_handle(curlMultiHandle, mCurlHandle);
    transferManager.registerForAsyncCallback(*this);
    mTM = &transferManager;
    return true;
}

void Request::shareState(TransferManager &transferManager)
{
    curl_easy_setopt(mCurlHandle, CURLOPT_SHARE, transferManager.getCurlMultiHandle());
}

const string &Request::getUrl() const
{
    return mURL;
}

void Request::setUrl(const string &mUrl)
{
    mURL = mUrl;
}
