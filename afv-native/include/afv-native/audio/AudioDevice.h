/* audio/AudioDevice.h
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

#ifndef AFV_NATIVE_AUDIODEVICE_H
#define AFV_NATIVE_AUDIODEVICE_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>

#include "afv-native/audio/ISampleSink.h"
#include "afv-native/audio/ISampleSource.h"

namespace afv_native {
    namespace audio {
        /**
         * AudioDevice provides the Platform/Interface agnostic interface to the
         * sound devices.
         *
         * The actual audio device drivers extend this abstract class and implement
         * the constructor and device probe functions as necessary.
         */
        class AudioDevice {
        protected:
            std::shared_ptr<ISampleSink> mSink;
            std::mutex mSinkPtrLock;
            std::shared_ptr<ISampleSource> mSource;
            std::mutex mSourcePtrLock;


            /** Ensures data within the abstract is zeroed.   Should always be called via
             * the initialiser chain of any subclasses.
             */
            AudioDevice();

        public:
            /** Abstract API ID type - it is up to the implementing driver to ensure that
             * the mapping is uniform in any given session.  Persistent mapping of values
             * is not guaranteed between sound backends or successive executions of the
             * code.  A uniform method for determining API ID should be provided by the
             * driver.
             */
            typedef unsigned int Api;

            /** DeviceInfo is a uniform structure by which API implementations can return
             * information about known devices.  The "id" value should be used as an
             * argument to the constructor of the driver instance to set the desired device.
             */
            struct DeviceInfo {
                std::string name;
                std::string id;
                explicit DeviceInfo(std::string name, std::string id = "");
            };

            virtual ~AudioDevice();

            /** open() should open and start the playback and capture of the nominated audio
             * streams that this device is bound to.  If the streams are already playing,
             * then it should return success.
             *
             * @return true if open() was successful, false otherwise.
             */
            virtual bool open() = 0;

            /** close() should stop and shutdown the playback and capture of the nominated
             * audio streams.  If the streams are already stopped, it must do nothing.
             */
            virtual void close() = 0;

            /** setSource sets the ISampleSource for this AudioDevice.
             *
             * Any existing source will have it's pointer released.
             *
             * This can be set to the invalid/empty pointer to disable the source, in which
             * case the device should output silence.
             *
             * @param newSrc a shared_ptr to the new source.
             */
            virtual void setSource(std::shared_ptr<ISampleSource> newSrc);

            /** setSink sets the ISampleSink for this AudioDevice.
             *
             * Any existing sink will have its pointer released.
             *
             * This can be set to the invalid/empty pointer to disable the sink, in which
             * case the device should simply discard any samples received from the hardware.
             *
             * @param newSink a shared_ptr to the new Sink
             */
            virtual void setSink(std::shared_ptr<ISampleSink> newSink);

            /** OutputUnderflows is a monotonic counter of the number of playback buffer
             * underflows that have occurred since the AudioDevice was constructed.
             */
            std::atomic<uint32_t>   OutputUnderflows;

            /** InputOverflows is a monotonic counter of the number of recording buffer
             * overflows that have occurred since the AudioDevice was constructed.
             */
            std::atomic<uint32_t>   InputOverflows;

            /* default implementation hooks... */
            static std::map<Api,std::string> getAPIs();
            static std::map<int,DeviceInfo> getCompatibleInputDevicesForApi(AudioDevice::Api api);
            static std::map<int,DeviceInfo> getCompatibleOutputDevicesForApi(AudioDevice::Api api);
            static std::shared_ptr<AudioDevice> makeDevice(
                    const std::string &userStreamName,
                    const std::string &outputDeviceId,
                    const std::string &inputDeviceId,
                    Api audioApi=-1);
        };
    }
}


#endif //AFV_NATIVE_AUDIODEVICE_H
