/* audio/SinkFrameSizeAdjuster.cpp
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

#include "afv-native/audio/SinkFrameSizeAdjuster.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cassert>

using namespace afv_native::audio;

SinkFrameSizeAdjuster::SinkFrameSizeAdjuster(
        std::shared_ptr<ISampleSink> destSink, unsigned int sinkFrameSize):
        mDestinationSink(std::move(destSink)), mSourceFrameSize(sinkFrameSize),
        mSinkBufferOffset(0)
{
    const size_t bufferSize = frameSizeSamples * sizeof(SampleType);
    mSinkBuffer = new SampleType[bufferSize];
	if (nullptr != mSinkBuffer) {
		::memset(mSinkBuffer, 0, bufferSize);
	}
}

SinkFrameSizeAdjuster::~SinkFrameSizeAdjuster()
{
    delete[] mSinkBuffer;
}

void SinkFrameSizeAdjuster::putAudioFrame(const SampleType *bufferIn)
{
    size_t sourceOffset = 0;
    //precondition:  mSinkBufferOffset cannot be >= frameSizeSamples.  If it was
    // at the end of the last push, that's a bug.
    assert(mSinkBufferOffset < frameSizeSamples);

    const size_t space_left_in_buffer = frameSizeSamples - mSinkBufferOffset;
    // bind to the smaller of the samples we have left, and the size the source.
    size_t sample_copy_count = std::min<size_t>(space_left_in_buffer, mSourceFrameSize);
    // then copy those into our sample send buffer.
    memcpy(mSinkBuffer+mSinkBufferOffset, bufferIn, sample_copy_count * sizeof(SampleType));
    mSinkBufferOffset += sample_copy_count;
    sourceOffset += sample_copy_count;

    while (mSinkBufferOffset >= frameSizeSamples) {
        mDestinationSink->putAudioFrame(mSinkBuffer);
        mSinkBufferOffset = 0;

        // if there's any samples left in the frame, copy them up to a whole frame maximum.
        if ((mSourceFrameSize - sourceOffset) > 0) {
            sample_copy_count = std::min<size_t>(frameSizeSamples, mSourceFrameSize - sourceOffset);
            memcpy(mSinkBuffer + mSinkBufferOffset, bufferIn + sourceOffset, sample_copy_count * sizeof(SampleType));
            mSinkBufferOffset += sample_copy_count;
        }
    }
}

