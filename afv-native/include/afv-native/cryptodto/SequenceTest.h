/* cryptodto/SequenceTest.h
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

#ifndef AFV_NATIVE_SEQUENCETEST_H
#define AFV_NATIVE_SEQUENCETEST_H

#include <cstdint>
#include <cassert>

#include "afv-native/cryptodto/params.h"

namespace afv_native {
    namespace cryptodto {
        enum class ReceiveOutcome {
            /// OK indicates that the packet was within the window and if possible, it advanced normally.
                    OK = 0,
            /// Before indicates that the packet arrived after the window had already rolled forward and should be discarded, or has been previously seen within the current window
                    Before,
            /// Overflow indicates that the packet is in window, but forced a forward window movement to skip-over unreceived packets.
                    Overflow,
        };

        class SequenceTest {
        private:
            sequence_bitfield_t _bitfield;
            sequence_t _min;
            unsigned _window;

            void advanceWindow();
        public:
            SequenceTest(sequence_t start_sequence, unsigned window);

            /**
             * Received() indicates that the nominated sequence number was received, and advances the sliding window
             * appropriately.
             *
             * @param newSequence
             * @return ReceiveOutcome indicating where it appeared in the sequence.
             *
             */
            ReceiveOutcome Received(sequence_t newSequence);

            sequence_t GetNext() const;

            void reset();
        };
    }
}

#endif //AFV_NATIVE_SEQUENCETEST_H
