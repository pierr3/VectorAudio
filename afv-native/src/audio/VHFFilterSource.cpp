/* audio/VHFFilterSource.cpp
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

#include "afv-native/audio/VHFFilterSource.h"

using namespace afv_native::audio;

VHFFilterSource::VHFFilterSource()
{
    setupPresets();
}

/** transformFrame lets use apply this filter to a normal buffer, without following the sink/source flow.
 *
 * It always performs a copy of the data from In to Out at the very least.
 */
void VHFFilterSource::transformFrame(SampleType *bufferOut, SampleType const bufferIn[])
{
    for(unsigned int i = 0; i < frameSizeSamples; i++)
    {
        for(unsigned int band = 0; band < m_filters.size(); band++)
        {
            bufferOut[i] = m_filters[band].TransformOne(bufferIn[i]);
        }
    }
}

void VHFFilterSource::setupPresets()
{
    m_filters.push_back(BiQuadFilter::highPassFilter(sampleRateHz, 310, 0.35));
    m_filters.push_back(BiQuadFilter::peakingEQ(sampleRateHz, 450, 0.75, 17.0));
    m_filters.push_back(BiQuadFilter::peakingEQ(sampleRateHz, 1450, 1.0, 12.5));
    m_filters.push_back(BiQuadFilter::peakingEQ(sampleRateHz, 2000, 1.0, 12.5));
    m_filters.push_back(BiQuadFilter::lowPassFilter(sampleRateHz, 2500, 0.35));
}
