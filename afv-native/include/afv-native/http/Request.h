/* Request.h
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2015,2019 Christopher Collins
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

#ifndef AFV_NATIVE_REQUEST_H
#define AFV_NATIVE_REQUEST_H

#include <string>
#include <functional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "afv-native/http/http.h"

namespace afv_native {
    namespace http {
        class TransferManager;

        class Request {
        private:
            static size_t curlWriteCallback(char *buffer, size_t size, size_t nitems, void *userdata);
            static size_t curlReadCallback(char *buffer, size_t size, size_t nitems, void *userdata);
            static int curlTransferInfoCallback(
                    void *clientp,
                    curl_off_t dltotal,
                    curl_off_t dlnow,
                    curl_off_t ultotal,
                    curl_off_t ulnow);

            void transferInfoCallback(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
            size_t writeCallback(char *buffer, size_t size, size_t nitems);
            size_t readCallback(char *buffer, size_t size, size_t nitems);

        protected:
            Method mMethod;
            std::string mURL;
            bool mFollowRedirect;

            Progress mProgress;

            CURL *mCurlHandle;
            struct curl_slist *mHeaders;
            TransferManager *mTM;

            std::vector<unsigned char> mReq;
            size_t mReqBufOffset;

            int mRespStatusCode;
            std::string mRespContentType;

            std::vector<unsigned char> mResp;

            char mCurlErrorBuffer[CURL_ERROR_SIZE];

            int mDownloadTotal;
            int mDownloadProgress;

            int mUploadTotal;
            int mUploadProgress;

            std::function<void(Request *, bool)> mCompletionCallback;

            virtual bool setupHandle();

        public:
            Request(const std::string &url, Method method);
            /* no copy constructor - Request must not be copied as it would break the internal states. */
            Request(const Request &cpysrc) = delete;

            virtual ~Request();

            /** resets the Request state back to that before a request has been
             * performed.
             */
            virtual void reset();

            void setHeader(const std::string &header, const std::string &value);

            /** sets the Request Body to the data provided at buf of size len.
             *
             * @param buf pointer to the data
             * @param len length of the data in octets.
             */
            void setRequestBody(const unsigned char *buf, size_t len);

            /** sets the Request body to the provided string
             *
             * @param body string containing the body to send.
             */
            void setRequestBody(const std::string &body);

            /** sets the Request body to the provided serialised json value
             *
             * @param j the json value to send
             * @throws nlohmann::json_exception if a casting/typing error occured whilst
             *  serialising the json object.
             */
            void setRequestBody(const nlohmann::json &j);

            void setCompletionCallback(std::function<void(Request *, bool)> cb);

            void setFollowRedirect(bool follow);

            void shareState(TransferManager &transferManager);

            /** doSync() performs the request synchronously.
             *
             * @return true if the request completed successfully, false otherwise.
             *
             * @note if an error occured (false return), you can check the error
             *      reported via getError()
             */
            virtual bool doSync();

            /** doAsync() schedules the request to be performed asynchronously.
             *
             * @param transferManager the TransferManager instance to use to
             *      execute the request.
             * @return true if the request was successfully scheduled for execution.
             *    false if a configuration or other issue prevented it from being
             *    scheduled.
             */
            virtual bool doAsync(TransferManager &transferManager);

            /** returns the last CURL error reported.
             */
            std::string getCurlError() const;

            Progress getProgress() const;

            /** returns the HTTP status code received
             *
             * @return the HTTP status code.
             */
            int getStatusCode() const;

            /** returns the Content-Type for the response received
             *
             * @return the Content Type header value.
             */
            std::string getContentType() const;

            std::string getResponseBody() const;

            void clearRequestBody();

            int getDownloadTotal() const;

            int getDownloadProgress() const;

            int getUploadTotal() const;

            int getUploadProgress() const;

            CURL *getCurlHandle() const;

            /** notifyTransferCompleted is invoked either by the synchronous method
             * or by the asynchronous scheduler to indicate that the transfer for this
             * request completed.
             *
             * Subclasses can extend this to generate notifications or to drive
             * state machines as necessary.
             */
            virtual void notifyTransferCompleted();

            /** notifyTransferError is invoked either by the synchronous method
             * or by the asynchronous scheduler to indicate that the transfer for this
             * request failed.
             *
             * Subclasses can extend this to generate notifications or to drive
             * state machines as necessary.
             */
            virtual void notifyTransferError();

            const std::string &getUrl() const;
            void setUrl(const std::string &mUrl);
        };
    }
}

#endif //AFV_NATIVE_REQUEST_H
