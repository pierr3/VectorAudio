/* crypdto/SequenceTest.cpp
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

#include "afv-native/cryptodto/SequenceTest.h"

#include <algorithm>

#if defined(_MSC_VER)

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)
#endif

using namespace std;
using namespace afv_native::cryptodto;

SequenceTest::SequenceTest(sequence_t start_sequence, unsigned window):
        _bitfield(0),
        _min(start_sequence),
        _window(window)
{
    if (_window < 1) {
        _window = 1;
    }
    if (_window > (sizeof(sequence_bitfield_t) * 8)) {
        _window = sizeof(sequence_bitfield_t) * 8;
    }
}

void
SequenceTest::advanceWindow()
{
    // bitflip the bitfield and look for the first set bit - this will be the
    // first unreceived packet in the window.
    unsigned long idx = 0;

#ifdef _MSC_VER
    auto found = _BitScanForward64(&idx, ~_bitfield);
    // if idx == 0, then the window was fully received and we advance the full amount!
    if (!found) {
        idx = sizeof(_bitfield) * 8;
    } else {
        idx++;
    }
#else
#ifdef __GNUC__
    idx = __builtin_ctz(~_bitfield) + 1;
#else
#error No BSF for this compiler defined.
#endif
#endif
    idx = std::min(idx, static_cast<unsigned long>(_window));
    _min += idx;
    _bitfield = _bitfield >> (idx);
}

ReceiveOutcome
SequenceTest::Received(sequence_t newSequence)
{
    if (newSequence < _min) {
        return ReceiveOutcome::Before;
    }
    if (newSequence == _min) {
        advanceWindow();
        return ReceiveOutcome::OK;
    }
    if (newSequence <= (_min + _window)) {
        // always at least 1 because if it was min, we'd have exited earlier.
        sequence_t bitidx = newSequence - _min - 1;

        sequence_bitfield_t mask = 1ULL << bitidx;
        if ((_bitfield & mask) == mask) {
            return ReceiveOutcome::Before;
        }
        _bitfield |= mask;
        return ReceiveOutcome::OK;
    }
    // if we're here, then we've forced a window jump.
    sequence_t oldmin = _min;
    while (_min < oldmin + _window) {
        advanceWindow();
        if ((_min + _window) > newSequence) {
            // if we're now in window, break out.
            break;
        }
    }
    // did we fully advance the window?  If so abandon the old position and restart the stream.
    if (_min >= oldmin + _window) {
        _min = newSequence + 1;
        _bitfield = 0;
        return ReceiveOutcome::Overflow;
    }
    // if we're inside the window now, mask and return.
    sequence_t bitidx = newSequence - _min - 1;
    sequence_bitfield_t mask = 1ULL << bitidx;
    _bitfield |= mask;
    return ReceiveOutcome::Overflow;
}

sequence_t
SequenceTest::GetNext() const
{
    return _min;
}

void SequenceTest::reset()
{
    _min = 0;
    _bitfield = 0;
}
