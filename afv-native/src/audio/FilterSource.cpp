/* audio/FilterSource.cpp
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2020 Christopher Collins
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

#include "afv-native/audio/FilterSource.h"

#include <memory>
#include <cstring>
#include "afv-native/audio/IFilter.h"

using namespace ::afv_native::audio;
using namespace ::std;

FilterSource::FilterSource(std::shared_ptr<ISampleSource> srcSource):
    mBypass(false),
    mUpstream(std::move(srcSource)),
    mFilters()
{
}

FilterSource::~FilterSource()
{
}

void FilterSource::addFilter(std::unique_ptr<IFilter> filter) {
    mFilters.emplace_back(std::move(filter));
}

SourceStatus FilterSource::getAudioFrame(SampleType *bufferOut) {
    if (mBypass) {
        return mUpstream->getAudioFrame(bufferOut);
    }
    SampleType thisFrame[frameSizeSamples];
    SourceStatus upstreamStatus = mUpstream->getAudioFrame(thisFrame);
    if (upstreamStatus != SourceStatus::OK) {
        memset(bufferOut, 0, frameSizeBytes);
        return upstreamStatus;
    }
    SampleType s = 0;
    for (unsigned i = 0; i < frameSizeSamples; i++) {
        s = thisFrame[i];
        for (auto &f: mFilters) {
            s = f->TransformOne(s);
        }
        bufferOut[i] = s;
    }
    return SourceStatus::OK;
}

void FilterSource::setBypass(bool bypass) {
    mBypass = bypass;
}

bool FilterSource::getBypass() const {
    return mBypass;
}
