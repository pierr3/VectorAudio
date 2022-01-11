/* afv/VoiceSession.h
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

#ifndef AFV_NATIVE_VOICESESSION_H
#define AFV_NATIVE_VOICESESSION_H

#include <string>
#include <event2/util.h>

#include "afv-native/afv/APISession.h"
#include "afv-native/afv/dto/VoiceServerConnectionData.h"
#include "afv-native/afv/dto/PostCallsignResponse.h"
#include "afv-native/afv/dto/Transceiver.h"
#include "afv-native/cryptodto/UDPChannel.h"
#include "afv-native/http/Request.h"
#include "afv-native/http/RESTRequest.h"
#include "afv-native/event/EventCallbackTimer.h"
#include "afv-native/util/monotime.h"
#include "afv-native/util/ChainedCallback.h"

namespace afv_native {
    namespace afv {
        class APISession;

        enum class VoiceSessionState {
            Connected,
            Disconnected,
            Error
        };

        enum class VoiceSessionError {
            NoError = 0,
            UDPChannelError,
            BadResponseFromAPIServer,
            Timeout,
        };

        class VoiceSession {
        public:
            util::ChainedCallback<void(VoiceSessionState)> StateCallback;

            VoiceSession(APISession &session, const std::string &callsign = "");
            virtual ~VoiceSession();

            void setCallsign(const std::string &newCallsign);

            bool isConnected() const;

            bool Connect();
            void Disconnect(bool do_close = true);
            void postTransceiverUpdate(
                    const std::vector<dto::Transceiver> &txDto,
                    std::function<void(http::Request *, bool)> callback);
            cryptodto::UDPChannel & getUDPChannel();

            VoiceSessionError getLastError() const;

        protected:
            APISession &mSession;
            std::string mCallsign;
            std::string mBaseUrl;

            void updateBaseUrl();

            cryptodto::UDPChannel mChannel;

            event::EventCallbackTimer mHeartbeatTimer;
            util::monotime_t mLastHeartbeatReceived;
            event::EventCallbackTimer mHeartbeatTimeout;

            VoiceSessionError mLastError;

            /** setupSession use the information in the PostCallsignResponse DTO to start up
             * the UDP session and tasks
             *
             * @param cresp the DTO detailing the session encryption keys, endpoint
             *      address, and other details.
             * @returns true if successful, false if the session couldn't be set up
             *
             * @note failures are usually caused by Network Socket problems - most other
             *     causes can't fail until their callback fires.
             */
            bool setupSession(const dto::PostCallsignResponse &cresp);

            /** failSession tears down the voice session and it's UDP channel due to failure,
             * timeout or explicit disconnect.
             */
            void failSession();

            void sendHeartbeatCallback();
            void receivedHeartbeat();
            void heartbeatTimedOut();

            void sessionStateCallback(APISessionState state);
            void voiceSessionSetupRequestCallback(http::Request *req, bool success);

            http::RESTRequest mVoiceSessionSetupRequest;
            http::RESTRequest mVoiceSessionTeardownRequest;
            http::RESTRequest mTransceiverUpdateRequest;
        };
    }
}

#endif //AFV_NATIVE_VOICESESSION_H
