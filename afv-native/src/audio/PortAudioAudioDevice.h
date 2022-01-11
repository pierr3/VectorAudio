/* audio/PortAudioAudioDevice.h
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

#ifndef AFV_NATIVE_PORTAUDIOAUDIODEVICE_H
#define AFV_NATIVE_PORTAUDIOAUDIODEVICE_H

#include "afv-native/audio/AudioDevice.h"

#include <portaudio/include/portaudio.h>

namespace afv_native {
    namespace audio {
        class PortAudioAudioDevice : public AudioDevice {
        private:
            static int paAudioCallback(
                    const void *inputBuffer,
                    void *outputBuffer,
                    unsigned long nFrames,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags status,
                    void *userData);
            int audioCallback(
                    const void *inputBuffer,
                    void *outputBuffer,
                    unsigned int nFrames,
                    const PaStreamCallbackTimeInfo *streamTime,
                    PaStreamCallbackFlags status);
        protected:
            unsigned mApi;
            std::string mUserStreamName;
            std::string mOutputDeviceName;
            std::string mInputDeviceName;

            bool mDidPortAudioInit;
            PaStream *mAudioDevice;

            bool getDeviceForName(const std::string &deviceName, bool forInput, PaStreamParameters &deviceParamOut);

        public:
            struct PrivateDeviceInfo {
                std::string name;
                int maxInputChannels;
                int maxOutputChannels;
                double lowInputLatency;
                double lowOutputLatency;
                double defaultSamplingRate;

                explicit PrivateDeviceInfo(const PaDeviceInfo *src);
            };

            explicit PortAudioAudioDevice(
                    const std::string &userStreamName,
                    const std::string &outputDeviceName,
                    const std::string &inputDeviceName,
                    Api audioApi);
            virtual ~PortAudioAudioDevice();
            bool open() override;
            void close() override;

        protected:
            static bool isAbleToOpen(int deviceId, int forInput=false);
        public:
            static std::map<Api,std::string> getAPIs();
            static std::map<int, PrivateDeviceInfo> getCompatibleInputDevicesForApi(AudioDevice::Api api);
            static std::map<int, PrivateDeviceInfo> getCompatibleOutputDevicesForApi(AudioDevice::Api api);
        };
    }
}
#endif /* AFV_NATIVE_PORTAUDIOAUDIODEVICE_H */
