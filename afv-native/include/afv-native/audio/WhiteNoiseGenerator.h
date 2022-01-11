/* audio/WhiteNoiseGenerator.h
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

#ifndef AFV_NATIVE_WHITENOISEGENERATOR_H
#define AFV_NATIVE_WHITENOISEGENERATOR_H

#include "afv-native/audio/ISampleSource.h"

namespace afv_native {
    namespace audio {

        /** WhiteNoiseGenerator generates, surprise surprise, White Noise.
         *
         * This is an implementation of the fast whitenoise generator by ed.bew@hcrikdlef.dreg as posted to musicdsp.
         */
        class WhiteNoiseGenerator: public ISampleSource {
        public:
            WhiteNoiseGenerator(float level = 1.0):
                mX1(0x67452301),
                mX2(0xefcdab89),
                mLevel(level)
            {}

            void setLevel(float newLevel)
            {
                mLevel = newLevel;
            }

            inline SampleType iterateOneSample() {
                SampleType sv;
                mX1 ^= mX2;
                sv = mX2 * mLevel * sScale;
                mX2 += mX1;
                return sv;
            }

            SourceStatus getAudioFrame(SampleType *bufferOut) override
            {
                size_t ctrLeft = frameSizeSamples;
                while (ctrLeft--) {
                    *(bufferOut++) = iterateOneSample();
                }
                return SourceStatus::OK;
            }

        protected:
            int32_t mX1;
            int32_t mX2;
            float mLevel;
            static constexpr float sScale = 2.0f / 0xffffffff;
        };
    }
}

#endif //AFV_NATIVE_WHITENOISEGENERATOR_H
