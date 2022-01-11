/* audio/SpeexPreprocessor.cpp
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

#include "afv-native/audio/SpeexPreprocessor.h"
#include "afv-native/audio/audio_params.h"
#include "afv-native/Log.h"

using namespace afv_native::audio;

SpeexPreprocessor::SpeexPreprocessor(std::shared_ptr<ISampleSink> upstream):
    mUpstreamSink(std::move(upstream)),
    mPreprocessorState(nullptr),
    mSpeexFrame(),
    mOutputFrame()
{
    mPreprocessorState = speex_preprocess_state_init(frameSizeSamples, sampleRateHz);

    int iarg = 1;
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_AGC, &iarg);
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_DENOISE, &iarg);
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_DEREVERB, &iarg);

    iarg = 21747;
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_AGC_TARGET, &iarg);

    iarg = 80;
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &iarg);

    iarg = -60;
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &iarg);

    iarg = -30;
    speex_preprocess_ctl(mPreprocessorState, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &iarg);
}

SpeexPreprocessor::~SpeexPreprocessor()
{
    speex_preprocess_state_destroy(mPreprocessorState);
}

void SpeexPreprocessor::putAudioFrame(const SampleType *bufferIn)
{
    for (size_t i = 0; i < frameSizeSamples; i++) {
        mSpeexFrame[i] = static_cast<spx_int16_t>(bufferIn[i] * 32767.0f);
    }
    speex_preprocess_run(mPreprocessorState, mSpeexFrame);
    for (size_t i = 0; i < frameSizeSamples; i++) {
        mOutputFrame[i] = static_cast<float>(mSpeexFrame[i]) / 32768.0f;
    }
    if (mUpstreamSink) {
        mUpstreamSink->putAudioFrame(mOutputFrame);
    }
}
