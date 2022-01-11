/* util/ChainedCallback.h
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

#ifndef AFV_NATIVE_CHAINEDCALLBACK_H
#define AFV_NATIVE_CHAINEDCALLBACK_H

#include <unordered_map>
#include <functional>

namespace afv_native {
    namespace util {

        template<class>
        class ChainedCallback;

        /** ChainedCallback provides a way to template no-return-value callbacks
         * with a owner-reference for safe cleanup in complex class heirachies.
         *
         * @tparam Args the argument list for the callback function.
         */
        template<class... Args>
        class ChainedCallback<void(Args...)> {
            std::unordered_map<void *, std::function<void(Args...)> > mCallbacks;
        public:
            ChainedCallback():
                    mCallbacks()
            {
            }

            virtual ~ChainedCallback() = default;

            void addCallback(void *ref, std::function<void(Args...)> fp)
            {
                mCallbacks[ref] = fp;
            }

            void removeCallback(void *ref)
            {
                mCallbacks.erase(ref);
            }

            void invokeAll(Args... args)
            {
                for (auto &f: mCallbacks) {
#ifdef __GNUC__
                    std::__invoke(f.second, std::forward<Args>(args)...);
#else
                    std::invoke(f.second, std::forward<Args>(args)...);
#endif
                }
            }
        };
    }
}


#endif //AFV_NATIVE_CHAINEDCALLBACK_H
