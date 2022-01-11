/* afv/RemoteVoiceSource.cpp
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

#include "afv-native/afv/RemoteVoiceSource.h"

#include <cstring>
#include <algorithm>

#include "afv-native/Log.h"
#include "afv-native/audio/audio_params.h"
#include "afv-native/util/monotime.h"

using namespace afv_native::afv;
using namespace afv_native::audio;
using namespace afv_native;
using namespace std;

RemoteVoiceSource::RemoteVoiceSource():
        mJitterBufferMutex(),
        mIsActive(false),
        mSilentFrames(0),
        mEnding(false),
        mEndingSequence(0),
        mCurrentFrame(0)
{
    mJitterBuffer = jitter_buffer_init(1);
    jitter_buffer_ctl(mJitterBuffer, JITTER_BUFFER_SET_DESTROY_CALLBACK, reinterpret_cast<void *>(::free));

    spx_uint32_t jitterMargin = 0;
    jitter_buffer_ctl(mJitterBuffer, JITTER_BUFFER_SET_MARGIN, &jitterMargin);

    int opus_status;
    mDecoder = opus_decoder_create(sampleRateHz, 1, &opus_status);
    if (opus_status != OPUS_OK) {
        LOG("instreambuffer", "Got error initialising Opus Codec: %s", opus_strerror(opus_status));
        mDecoder = nullptr;
    }
}

RemoteVoiceSource::~RemoteVoiceSource()
{
    if (mDecoder != nullptr) {
        opus_decoder_destroy(mDecoder);
        mDecoder = nullptr;
    }
    jitter_buffer_destroy(mJitterBuffer);
    mJitterBuffer = nullptr;
}

void RemoteVoiceSource::appendAudioDTO(const dto::IAudio &audio)
{
    JitterBufferPacket newPacket;
    ::memset(&newPacket, 0, sizeof(newPacket));

    if (audio.LastPacket) {
        mEnding = true;
        mEndingSequence = audio.SequenceCounter;
    } else {
        mEnding = false;
    }

    newPacket.data = static_cast<char *>(::malloc(audio.Audio.size()));
    memcpy(newPacket.data, audio.Audio.data(), audio.Audio.size());
    newPacket.len = audio.Audio.size();
    newPacket.timestamp = audio.SequenceCounter;
    newPacket.span = 1;
    {
        std::lock_guard<std::mutex> lock(mJitterBufferMutex);

        jitter_buffer_put(mJitterBuffer, &newPacket);
        mSilentFrames = 0;
        mLastActive = util::monotime_get();
    }
    mIsActive = true;
}

SourceStatus RemoteVoiceSource::getAudioFrame(SampleType *bufferOut)
{
    SourceStatus rv = SourceStatus::OK;
    JitterBufferPacket pktOut;
    ::memset(&pktOut, 0, sizeof(pktOut));

    spx_int32_t tsOut;
    int jitter_status;
    int opus_res = OPUS_OK;
    {
        std::lock_guard<std::mutex> lock(mJitterBufferMutex);
        jitter_status = jitter_buffer_get(mJitterBuffer, &pktOut, 1, &tsOut);
    }
    if (mDecoder != nullptr) {
        switch (jitter_status) {
        case JITTER_BUFFER_MISSING:
            mCurrentFrame++;
            if (mEnding && (mCurrentFrame >= mEndingSequence)) {
                ::memset(bufferOut, 0, frameSizeSamples * sizeof(SampleType));
                rv = SourceStatus::Closed;
            } else {
                // prod opus to perform gap compensation.
                opus_res = opus_decode_float(mDecoder, nullptr, 0, bufferOut, frameSizeSamples, false);
            }
            break;
        case JITTER_BUFFER_INSERTION:
            // insert silence.
            ::memset(bufferOut, 0, frameSizeSamples * sizeof(SampleType));
            break;
        case JITTER_BUFFER_OK:
            mCurrentFrame = tsOut;
            opus_res = opus_decode_float(
                    mDecoder,
                    reinterpret_cast<unsigned char *>(pktOut.data),
                    pktOut.len,
                    bufferOut,
                    frameSizeSamples,
                    false);
                ::free(pktOut.data);
            break;
        default:
            LOG("instreambuffer", "Got Error return from the jitter buffer: %d", jitter_status);
            rv = SourceStatus::Error;
            break;
        }
        if (opus_res < 0) {
            LOG("instreambuffer", "Opus returned an error decoding frame: %s", opus_strerror(opus_res));
        }
    } else {
        // codec is broken - insert silence.
        memset(bufferOut, 0, frameSizeSamples * sizeof(SampleType));
        rv = SourceStatus::Error;
    }
    {
        std::lock_guard<std::mutex> lock(mJitterBufferMutex);
        jitter_buffer_tick(mJitterBuffer);
        // if we don't have a terminally flagged marker, check for timeouts.
        spx_int32_t bufCount = 0;
        jitter_buffer_ctl(mJitterBuffer, JITTER_BUFFER_GET_AVAILABLE_COUNT, &bufCount);
        if (bufCount == 0) {
            mSilentFrames += 1;
            if (mSilentFrames > frameTimeOut) {
                if (rv != SourceStatus::Error) {
                    rv = SourceStatus::Closed;
                }
            }
        }
    }
    if (rv != SourceStatus::OK) {
        mIsActive = false;
    }
    return rv;
}

void RemoteVoiceSource::flush()
{
    {
        std::lock_guard<std::mutex> lock(mJitterBufferMutex);
        // this nukes the jitter buffer contents, without resetting the latency timers.
        jitter_buffer_reset(mJitterBuffer);
    }
    opus_decoder_ctl(mDecoder, OPUS_RESET_STATE);
}

bool RemoteVoiceSource::isActive() const
{
    return mIsActive;
}

util::monotime_t RemoteVoiceSource::getLastActivityTime() const
{
    return mLastActive;
}

