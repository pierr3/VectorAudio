/* audio/PinkNoiseGenerator.h
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

#ifndef AFV_NATIVE_PINKNOISEGENERATOR_H
#define AFV_NATIVE_PINKNOISEGENERATOR_H

#include "afv-native/audio/ISampleSource.h"
#include "afv-native/audio/WhiteNoiseGenerator.h"

namespace afv_native {
    namespace audio {
        /** PinkNoiseGenerator generates pink noise.
         *
         * The Pink Noise is based on a filter by Paul Kellett published on musicdsp.org.  It incorporates the downscale
         * "fix" from NAudio.
         */
        class PinkNoiseGenerator: public ISampleSource {
        public:
            explicit PinkNoiseGenerator(float gain = 1.0):
                white(),
                mGain(gain),
                mB{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
            {}

            inline SampleType iterateOneSample() {
                SampleType w = white.iterateOneSample();

                mB[0] = 0.99886 * mB[0] + w * 0.0555179;
                mB[1] = 0.99332 * mB[1] + w * 0.0750759;
                mB[2] = 0.96900 * mB[2] + w * 0.1538520;
                mB[3] = 0.86650 * mB[3] + w * 0.3104856;
                mB[4] = 0.55000 * mB[4] + w * 0.5329522;
                mB[5] = -0.7616 * mB[5] - w * 0.0168980;
                SampleType pOut = mB[0] + mB[1] + mB[2] + mB[3] + mB[4] + mB[5] + mB[6] + w * 0.5362;
                mB[6] = w * 0.115926;
                return (pOut/5.0f) * mGain;
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
            WhiteNoiseGenerator white;
            float mGain;

            SampleType mB[7];
        };
    }
}


#endif //AFV_NATIVE_PINKNOISEGENERATOR_H
