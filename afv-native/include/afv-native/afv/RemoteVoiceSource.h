/* afv/RemoteVoiceSource.h
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

#ifndef AFV_NATIVE_REMOTEVOICESOURCE_H
#define AFV_NATIVE_REMOTEVOICESOURCE_H

#include <mutex>
#include <speex/speex_jitter.h>
#include <opus/include/opus.h>

#include "afv-native/afv/dto/interfaces/IAudio.h"
#include "afv-native/audio/audio_params.h"
#include "afv-native/audio/ISampleSource.h"
#include "afv-native/audio/SourceStatus.h"
#include "afv-native/util/monotime.h"

namespace afv_native {
    namespace afv {

        /** frameTimeOut is the maximum number of frames we will receive without audio data before we declare the
         * stream dead.
         */
        const int frameTimeOut = 10;

        /** RemoveVoiceSource takes a stream of IAudio DTOs and stores them in an appropriately tuned jitterbuffer.
         *
         * These can then be demand polled by a consumer which will pull the packets from the jitterBuffer and run them
         * through the decoder.
         *
         * @note this is analogous to the GeoVR CallsignSampleProvider, but without the effects pass which is handled
         * elsewhere.
         */
        class RemoteVoiceSource: public audio::ISampleSource {
        protected:
            JitterBuffer *mJitterBuffer;
            OpusDecoder *mDecoder;

            std::mutex mJitterBufferMutex;
            bool mIsActive;
            util::monotime_t mLastActive;
        protected:
            int mSilentFrames;

            int mCurrentFrame;
            bool mEnding;
            int mEndingSequence;
        public:
            RemoteVoiceSource();
            virtual ~RemoteVoiceSource();
            RemoteVoiceSource(const RemoteVoiceSource& copySrc) = delete;

            void appendAudioDTO(const dto::IAudio &audio);
            audio::SourceStatus getAudioFrame(audio::SampleType *bufferOut) override;

            util::monotime_t getLastActivityTime() const;

            /** flush resets the stream, preserving any jitter adjustments, but otherwise clearing the codec state and
             * jitter buffered packets.
             */
            void flush();
            bool isActive() const;
        };
    }
}

#endif //AFV_NATIVE_REMOTEVOICESOURCE_H
