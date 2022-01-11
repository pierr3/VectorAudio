/* audio/WavSampleStorage.cpp
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

#include "afv-native/audio/WavSampleStorage.h"

#include <vector>
#include <cstring>
#include <speex/speex_resampler.h>

using namespace afv_native::audio;
using namespace std;

typedef SampleType (*fetchSample)(const void *base, size_t stride, size_t offset, int channels);

SampleType convert8bit(const void *base, size_t stride, size_t offset, int channels)
{
    auto *sampleStart = reinterpret_cast<const uint8_t *>(base) + (stride * offset);
    float sampleOut = 0.0f;
    sampleOut = (static_cast<float>(sampleStart[0]) - 128.0f) / 128.0f;
    if (channels == 2) {
        sampleOut += (static_cast<float>(sampleStart[1]) - 128.0f) / 128.0f;
        sampleOut /= 2;
    }
    return sampleOut;
}

SampleType convert16bit(const void *base, size_t stride, size_t offset, int channels)
{
    auto *sampleStart = reinterpret_cast<const int16_t *>(reinterpret_cast<const char *>(base) + (stride * offset));
    float sampleOut = 0.0f;
    sampleOut = static_cast<float>(sampleStart[0]) / 32768.0f;
    if (channels == 2) {
        sampleOut += static_cast<float>(sampleStart[1]) / 32768.0f;
        sampleOut /= 2;
    }
    return sampleOut;
}

SampleType convertfloat(const void *base, size_t stride, size_t offset, int channels)
{
    auto *sampleStart = reinterpret_cast<const float *>(reinterpret_cast<const char *>(base) + (stride * offset));
    float sampleOut = sampleStart[0];
    if (channels == 2) {
        sampleOut += sampleStart[1];
        sampleOut /= 2;
    }
    return sampleOut;
}


WavSampleStorage::WavSampleStorage(const AudioSampleData &srcdata):
        mBuffer(nullptr),
        mBufferSize(0)
{
    fetchSample sampleFetch = nullptr;

    switch (srcdata.getBitsPerSample()) {
    case 8:
        sampleFetch = convert8bit;
        break;
    case 16:    	
        sampleFetch = convert16bit;
        break;
    case 32:
    	if (srcdata.isFloat()) {
			sampleFetch = convertfloat;
		}
        break;
    default:
        return;
    }
    const size_t len = srcdata.getSampleCount();
    const void *sData = srcdata.getSampleData();
    const int stride = srcdata.getSampleAlignment();
    const int channels = srcdata.getNumChannels();
    if (srcdata.getSampleRate() != sampleRateHz) {
        std::vector<SampleType> convertBuffer(len);
        for (auto i = 0; i < len; i++) {
            convertBuffer[i] = sampleFetch(sData, stride, i, channels);
        }
        mBufferSize = len * sampleRateHz / srcdata.getSampleRate();
        mBuffer = new SampleType[mBufferSize];
        int res;
        auto resampler = speex_resampler_init(1, srcdata.getSampleRate(), sampleRateHz, SPEEX_RESAMPLER_QUALITY_DESKTOP, &res);
        speex_resampler_skip_zeros(resampler);
        spx_uint32_t resampleLen = static_cast<spx_uint32_t>(len);
        spx_uint32_t outputLen = static_cast<spx_uint32_t>(mBufferSize);
        speex_resampler_process_float(resampler, 0, convertBuffer.data(), &resampleLen, mBuffer, &outputLen);
        mBufferSize = outputLen;
        speex_resampler_destroy(resampler);
    } else {
        mBufferSize = srcdata.getSampleCount();
        mBuffer = new SampleType[srcdata.getSampleCount()];
        for (auto i = 0; i < len; i++) {
            mBuffer[i] = sampleFetch(sData, stride, i, channels);
        }
    }
}

WavSampleStorage::WavSampleStorage(const WavSampleStorage &copysrc)
{
    mBuffer = new SampleType[copysrc.mBufferSize];
    ::memcpy(mBuffer, copysrc.mBuffer, copysrc.mBufferSize * sizeof(SampleType));
    mBufferSize = copysrc.mBufferSize;
}

WavSampleStorage::WavSampleStorage(WavSampleStorage &&movesrc) noexcept
{
    mBuffer = movesrc.mBuffer;
    movesrc.mBuffer = nullptr;
    mBufferSize = movesrc.mBufferSize;
    movesrc.mBufferSize = 0;
}

WavSampleStorage::~WavSampleStorage()
{
    if (mBuffer) {
        delete[] mBuffer;
        mBuffer = nullptr;
    }
}

SampleType *WavSampleStorage::data() const
{
    return mBuffer;
}

size_t WavSampleStorage::lengthInSamples() const
{
    return mBufferSize;
}

WavSampleStorage& WavSampleStorage::operator=(const WavSampleStorage& copySrc)
{
    if (mBufferSize != copySrc.mBufferSize)
    {
        if (mBuffer)
        {
            delete[] mBuffer;
        }
        mBuffer = new SampleType[copySrc.mBufferSize];
        mBufferSize = copySrc.mBufferSize;
    }
    ::memcpy(mBuffer, copySrc.mBuffer, copySrc.mBufferSize * sizeof(SampleType));
    return *this;
}

WavSampleStorage& WavSampleStorage::operator=(WavSampleStorage&& moveSrc) noexcept
{
    if (mBuffer)
    {
        delete[] mBuffer;
        mBufferSize = 0;
    }
    mBuffer = moveSrc.mBuffer;
    mBufferSize = moveSrc.mBufferSize;
    moveSrc.mBuffer = nullptr;
    moveSrc.mBufferSize = 0;
    return *this;
}