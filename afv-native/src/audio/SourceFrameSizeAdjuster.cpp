/* audio/SourceFrameSizeAdjuster.cpp
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

#include "afv-native/audio/SourceFrameSizeAdjuster.h"
#include <memory>
#include <algorithm>
#include <cstring>

using namespace afv_native::audio;
using namespace std;

SourceStatus SourceFrameSizeAdjuster::getAudioFrame(SampleType *bufferOut)
{
    if (!mOriginSource) {
        return SourceStatus::Closed;
    }
    size_t destOffset = 0;
    // use residual frame first.
    if (mSourceBufferOffset > 0) {
        size_t framesToCopy = std::min<size_t>(frameSizeSamples - mSourceBufferOffset, mDestinationFrameSize);
        ::memcpy(bufferOut + destOffset, mSourceBuffer + mSourceBufferOffset, framesToCopy * sizeof(SampleType));
        mSourceBufferOffset = (mSourceBufferOffset + framesToCopy) % frameSizeSamples;
        destOffset += framesToCopy;
    }
    SourceStatus sourceRes;
    while (destOffset < mDestinationFrameSize) {
        const size_t samplesRemaining = mDestinationFrameSize - destOffset;

        if (samplesRemaining > frameSizeSamples) {
            // shortcut by writing straight into the buffer.
            sourceRes = mOriginSource->getAudioFrame(bufferOut + destOffset);
            if (sourceRes != SourceStatus::OK) {
                // something broke.  silencefill the buffer, and return OK, but kill our source handle.
                ::memset(bufferOut+destOffset, 0, sizeof(SampleType) * samplesRemaining);
                mOriginSource.reset();
                return SourceStatus::OK;
            }
            destOffset += frameSizeSamples;
        } else {
            // the remaining samples to copy must be less than a frame.
            // fill our holding buffer.
            sourceRes = mOriginSource->getAudioFrame(mSourceBuffer);
            mSourceBufferOffset = 0;
            if (sourceRes != SourceStatus::OK) {
                // something broke.  silencefill the buffer, and return OK, but kill our source handle.
                ::memset(bufferOut + destOffset, 0, sizeof(SampleType) * samplesRemaining);
                mOriginSource.reset();
                return SourceStatus::OK;
            }
            // now, copy out the difference.
            ::memcpy(bufferOut + destOffset, mSourceBuffer, samplesRemaining * sizeof(SampleType));
            mSourceBufferOffset = samplesRemaining;
            destOffset += samplesRemaining;
        }
    }
    return SourceStatus::OK;
}

SourceFrameSizeAdjuster::SourceFrameSizeAdjuster(
        std::shared_ptr<ISampleSource> originSource, unsigned int outputFrameSize):
        mOriginSource(std::move(originSource)), mDestinationFrameSize(outputFrameSize), mSourceBufferOffset(0)
{
    const size_t bufferSize = frameSizeSamples * sizeof(SampleType);
    mSourceBuffer = new SampleType[bufferSize];
    ::memset(mSourceBuffer, 0, bufferSize);
}

SourceFrameSizeAdjuster::~SourceFrameSizeAdjuster()
{
    delete[] mSourceBuffer;
}
