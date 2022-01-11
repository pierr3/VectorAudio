/* afv/RollingAverage.h
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

#ifndef AFV_NATIVE_ROLLINGAVERAGE_H
#define AFV_NATIVE_ROLLINGAVERAGE_H

#include <vector>
#include <cstddef>

namespace afv_native {
    namespace afv {
        template<class T>
        class RollingAverage {
        protected:
            size_t mAverageWindow;
            std::vector<T> mData;
            size_t mDOffset;
            T mRollingAverage;
        public:
            explicit RollingAverage(size_t length):
                    mAverageWindow(length),
                    mData(length),
                    mDOffset(0),
                    mRollingAverage()
            {
            }

            T addDatum(T next)
            {
                mRollingAverage += (next - mData[mDOffset]) / mAverageWindow;
                mData[mDOffset] = next;
                mDOffset = (mDOffset + 1) % mAverageWindow;

                return mRollingAverage;
            }

            T getAverage() const
            {
                return mRollingAverage;
            }

            T getLast() const
            {
                return mData[(mDOffset + mAverageWindow - 1) % mAverageWindow];
            }

            T getMax() const
            {
                T lMax = mData[0];
                for (size_t i = 1; i < mAverageWindow; i++) {
                    lMax = std::max<T>(lMax, mData[i]);
                }
                return lMax;
            }

            T getMin() const
            {
                T lMin = mData[0];
                for (size_t i = 1; i < mAverageWindow; i++) {
                    lMin = std::min<T>(lMin, mData[i]);
                }
                return lMin;
            }
        };
    }
}

#endif //AFV_NATIVE_ROLLINGAVERAGE_H
