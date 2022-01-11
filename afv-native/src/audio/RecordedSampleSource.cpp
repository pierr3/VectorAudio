/* audio/RecordedSampleSource.cpp
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

#include <algorithm>
#include <cstring>
#include "afv-native/audio/RecordedSampleSource.h"

using namespace afv_native::audio;
using namespace std;

SourceStatus RecordedSampleSource::getAudioFrame(SampleType *bufferOut)
{
    size_t bufOffset = 0;
    if (!mPlay || !mSampleSource) {
        return SourceStatus::Closed;
    }
    const size_t sourceLength = mSampleSource->lengthInSamples();
    do {
        if (mLoop && mCurPosition >= sourceLength) {
            mCurPosition = 0;
        }
        auto maxCopy = min<size_t>(frameSizeSamples - bufOffset, sourceLength - mCurPosition);
        ::memcpy(bufferOut + bufOffset, mSampleSource->data() + mCurPosition, maxCopy * sizeof(SampleType));
        mCurPosition += maxCopy;
        bufOffset += maxCopy;
    } while (mLoop && bufOffset < frameSizeSamples);
    if (bufOffset < frameSizeSamples) {
        const size_t fillSize = frameSizeSamples - bufOffset;
        ::memset(bufferOut + bufOffset, 0, sizeof(SampleType)*fillSize);
    }
    if (!mLoop && mCurPosition >= mSampleSource->lengthInSamples()) {
        mPlay = false;
    }
    return SourceStatus::OK;
}

RecordedSampleSource::RecordedSampleSource(const std::shared_ptr<ISampleStorage> src, bool loop):
    mSampleSource(src),
    mLoop(loop),
    mPlay(true),
    mCurPosition(0)
{
}

RecordedSampleSource::~RecordedSampleSource()
{
}

bool RecordedSampleSource::isPlaying() const
{
    return mPlay;
}
