/* WavFile.h
 *
 * Class(es) for reading audio samples
 *
 * Copyright (C) 2018, Christopher Collins
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

/*
 * This module is only intended to support a narrow subset of possible Wav 
 * files, namely: PCM only.
*/

#ifndef AFV_NATIVE_WAVFILE_H
#define AFV_NATIVE_WAVFILE_H

#include <cstdlib>
#include <cstdint>

namespace afv_native {
    namespace audio {
        class AudioSampleData {
        protected:
            int8_t mNumChannels;
            int8_t mBitsPerSample;
            uint8_t mSampleAlignment;
            int mSampleRate;

            unsigned mSampleCount;
            void *mSampleData;

            bool mIsFloat;

        public:
            AudioSampleData(int numChannels, int bitsPerSample, int sampleRate, bool isFloat = false);
            AudioSampleData(AudioSampleData &&move_src) noexcept;
            AudioSampleData(const AudioSampleData &cpy_src);
            virtual            ~AudioSampleData();

            int8_t getNumChannels() const;
            int8_t getBitsPerSample() const;
            uint8_t getSampleAlignment() const;
            int getSampleRate() const;
            size_t getSampleCount() const;
            const void *getSampleData() const;
            bool isFloat() const;
            void AppendSamples(uint8_t blockSize, unsigned count, void *data);
        };

        AudioSampleData *LoadWav(const char *fileName);
    }
}


#endif /* AFV_NATIVE_WAVFILE_H */