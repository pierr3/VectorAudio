/* afv/VoiceCompressionSink.cpp
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

#include "afv-native/afv/VoiceCompressionSink.h"

#include <vector>

#include "afv-native/Log.h"

using namespace ::afv_native;
using namespace ::afv_native::afv;
using namespace ::std;

VoiceCompressionSink::VoiceCompressionSink(ICompressedFrameSink &sink):
		mEncoder(nullptr),
        mCompressedFrameSink(sink)
{
    open();
}

VoiceCompressionSink::~VoiceCompressionSink()
{
    close();
}

int VoiceCompressionSink::open()
{
    int opus_status = 0;
    if (mEncoder != nullptr) {
        return 0;
    }
    mEncoder = opus_encoder_create(audio::sampleRateHz, 1, OPUS_APPLICATION_VOIP, &opus_status);
    if (opus_status != OPUS_OK) {
        LOG("VoiceCompressionSink", "Got error initialising Opus Codec: %s", opus_strerror(opus_status));
        mEncoder = nullptr;
    } else {
        opus_status = opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(audio::encoderBitrate));
        if (opus_status != OPUS_OK) {
            LOG("VoiceCompressionSink", "error setting bitrate on codec: %s", opus_strerror(opus_status));
        }
    }
    return opus_status;
}

void VoiceCompressionSink::close()
{
    if (nullptr != mEncoder) {
        opus_encoder_destroy(mEncoder);
        mEncoder = nullptr;
    }
}

void VoiceCompressionSink::reset()
{
    close();
    open();
}

void VoiceCompressionSink::putAudioFrame(const audio::SampleType *bufferIn)
{
    vector<unsigned char> outBuffer(audio::targetOutputFrameSizeBytes);
    auto enc_len = opus_encode_float(mEncoder, bufferIn, audio::frameSizeSamples, outBuffer.data(), outBuffer.size());
    if (enc_len < 0) {
        LOG("VoiceCompressionSink", "error encoding frame: %s", opus_strerror(enc_len));
        return;
    }
    outBuffer.resize(enc_len);
    mCompressedFrameSink.processCompressedFrame(outBuffer);
}

