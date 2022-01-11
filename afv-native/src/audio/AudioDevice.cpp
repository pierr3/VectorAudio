/* audio/AudioDevice.cpp
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

#include "afv-native/audio/AudioDevice.h"

#include <memory>
#include <cstring>
#include <algorithm>

#include "afv-native/Log.h"

using namespace afv_native::audio;
using namespace std;

AudioDevice::AudioDevice():
    mSink(),
    mSinkPtrLock(),
    mSource(),
    mSourcePtrLock(),
    OutputUnderflows(0),
    InputOverflows(0)
{
}

AudioDevice::~AudioDevice()
{
}

void AudioDevice::setSource(std::shared_ptr<ISampleSource> newSrc) {
    std::lock_guard<std::mutex> sourceGuard(mSourcePtrLock);

    mSource = std::move(newSrc);
}

void AudioDevice::setSink(std::shared_ptr<ISampleSink> newSink) {
    std::lock_guard<std::mutex> sinkGuard(mSinkPtrLock);

    mSink = std::move(newSink);
}

AudioDevice::DeviceInfo::DeviceInfo(std::string newName, std::string newId) :
        name(std::move(newName)),
        id(std::move(newId))
{
    if (id.empty()) {
        id = name;
    }
}
