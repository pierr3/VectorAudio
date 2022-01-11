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

#include "SoundIOAudioDevice.h"

#include <memory>
#include <cstring>
#include <algorithm>

#include "afv-native/Log.h"

using namespace afv_native::audio;
using namespace std;

SoundIOAudioDevice::SoundIOAudioDevice(
        const std::string &userStreamName,
        const std::string &outputDeviceName,
        const std::string &inputDeviceName,
        AudioDevice::Api audioApi) :
        AudioDevice(),
        mApi(audioApi),
        mUserStreamName(userStreamName),
        mOutputDeviceName(outputDeviceName),
        mInputDeviceName(inputDeviceName),
        mSoundIO(),
        mInputStream(),
        mOutputStream(),
        mInputRingBuffer(),
        mOutputRingBuffer()
{
    mSoundIO = soundio_create();
    if (mSoundIO == nullptr) {
        LOG("SoundIOAudioDevice", "libsoundio failed to create context");
    } else {
        mSoundIO->app_name = mUserStreamName.c_str();
        if (mApi < 0) {
            auto rv = soundio_connect(mSoundIO);
            if (rv != SoundIoErrorNone) {
                LOG("SoundIOAudioDevice", "failed to connect to default API: %s", soundio_strerror(rv));
            }
        } else {
            auto backend = static_cast<enum SoundIoBackend>(mApi);
            auto rv = soundio_connect_backend(mSoundIO, backend);
            if (rv != SoundIoErrorNone) {
                LOG("SoundIOAudioDevice", "failed to connect to API %s: %s", soundio_backend_name(backend),
                    soundio_strerror(rv));
            }
        }
        mInputRingBuffer = soundio_ring_buffer_create(mSoundIO, frameSizeBytes);
        mOutputRingBuffer = soundio_ring_buffer_create(mSoundIO, frameSizeBytes);
    }
}

SoundIOAudioDevice::~SoundIOAudioDevice() {
    if (mSoundIO != nullptr) {
        if (mInputStream != nullptr) {
            soundio_instream_pause(mInputStream, true);
            soundio_instream_destroy(mInputStream);
            mInputStream = nullptr;
        }
        if (mOutputStream != nullptr) {
            soundio_outstream_pause(mOutputStream, true);
            soundio_outstream_destroy(mOutputStream);
            mOutputStream = nullptr;
        }
        if (mInputRingBuffer != nullptr) {
            soundio_ring_buffer_destroy(mInputRingBuffer);
            mInputRingBuffer = nullptr;
        }
        if (mOutputRingBuffer != nullptr) {
            soundio_ring_buffer_destroy(mOutputRingBuffer);
        }
        soundio_destroy(mSoundIO);
        mSoundIO = nullptr;
    }
}

bool SoundIOAudioDevice::open() {
    auto *inputDevice = getInputDeviceForId(mInputDeviceName);
    auto *outputDevice = getOutputDeviceForId(mOutputDeviceName);
    auto *monoLayout = soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdMono);
    auto *stereoLayout = soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdStereo);

    if (inputDevice) {
        mInputStream = soundio_instream_create(inputDevice);
        if (mInputStream) {
            auto inputLayout = soundio_best_matching_channel_layout(monoLayout, 1, inputDevice->layouts,
                                                                    inputDevice->layout_count);
            mInputStream->layout = *inputLayout;
            mInputStream->format = SoundIoFormatFloat32NE;
            mInputStream->sample_rate = sampleRateHz;
            mInputStream->userdata = this;
            mInputStream->software_latency = static_cast<double>(frameLengthMs) / 1000.0;
            mInputStream->name = "AFV Microphone";
            mInputStream->read_callback = staticSioReadCallback;
            mInputStream->overflow_callback = staticSioInputOverflowCallback;
            mInputStream->error_callback = staticSioInputErrorCallback;
            auto rv = soundio_instream_open(mInputStream);
            if (rv != SoundIoErrorNone) {
                LOG("SoundIOAudioDevice::open()", "Couldn't open input stream: %s", soundio_strerror(rv));
                soundio_instream_destroy(mInputStream);
                mInputStream = nullptr;
                return false;
            }
            rv = soundio_instream_start(mInputStream);
            if (rv != SoundIoErrorNone) {
                LOG("SoundIOAudioDevice::open()", "Couldn't start input stream: %s", soundio_strerror(rv));
                soundio_instream_destroy(mInputStream);
                mInputStream = nullptr;
                return false;
            }
        }
    }

    if (outputDevice) {
        if (!soundio_device_supports_layout(outputDevice, monoLayout)) {
            mOutputIsStereo = true;
        } else {
            mOutputIsStereo = false;
        }
        mOutputStream = soundio_outstream_create(outputDevice);
        if (mOutputStream) {
            auto outputLayout = soundio_best_matching_channel_layout(mOutputIsStereo ? stereoLayout : monoLayout,
                                                                     1,
                                                                     outputDevice->layouts,
                                                                     outputDevice->layout_count);
            mOutputStream->layout = *outputLayout;
            mOutputStream->format = SoundIoFormatFloat32NE;
            mOutputStream->sample_rate = sampleRateHz;
            mOutputStream->userdata = this;
            mOutputStream->software_latency = static_cast<double>(frameLengthMs) / 1000.0;
            mOutputStream->name = "AFV Radio Speaker";
            mOutputStream->write_callback = staticSioWriteCallback;
            mOutputStream->underflow_callback = staticSioOutputUnderflowCallback;
            mOutputStream->error_callback = staticSioOutputErrorCallback;
            auto rv = soundio_outstream_open(mOutputStream);
            if (rv != SoundIoErrorNone) {
                LOG("SoundIOAudioDevice::open()", "Couldn't open output stream: %s", soundio_strerror(rv));
                soundio_outstream_destroy(mOutputStream);
                mOutputStream = nullptr;
                return false;
            }
            rv = soundio_outstream_start(mOutputStream);
            if (rv != SoundIoErrorNone) {
                LOG("SoundIOAudioDevice::open()", "Couldn't start output stream: %s", soundio_strerror(rv));
                soundio_outstream_destroy(mOutputStream);
                mOutputStream = nullptr;
                return false;
            }
        }
    }
    return true;
}

void SoundIOAudioDevice::sioWriteCallback(struct SoundIoOutStream *stream, int frame_count_min, int frame_count_max) {
    std::lock_guard<std::mutex> sourceLock(mSourcePtrLock);

    int ringFrames = soundio_ring_buffer_fill_count(mOutputRingBuffer) / static_cast<int>(sizeof(SampleType));
    const size_t frames = optimumFrameCount(ringFrames, frame_count_min, frame_count_max);

    SoundIoChannelArea *bufAreas;
    int sampleCount = static_cast<int>(frames);
    auto rv = soundio_outstream_begin_write(stream, &bufAreas, &sampleCount);
    if (rv == SoundIoErrorNone) {
        char *chPtr[2] = {bufAreas[0].ptr, bufAreas[0].ptr};
        int chStep[2] = {bufAreas[0].step, bufAreas[0].step};
        if (mOutputIsStereo) {
            chPtr[1] = bufAreas[1].ptr;
            chStep[1] = bufAreas[1].step;
        }
        int samplesWritten = 0;
        while (samplesWritten < sampleCount) {
            ringFrames = soundio_ring_buffer_fill_count(mOutputRingBuffer) / static_cast<int>(sizeof(SampleType));
            if (ringFrames == 0) {
                auto *sourceFillPtr = reinterpret_cast<SampleType *>(soundio_ring_buffer_write_ptr(mOutputRingBuffer));
                if (mSource) {
                    auto sourcerv = mSource->getAudioFrame(sourceFillPtr);
                    if (sourcerv != SourceStatus::OK) {
                        ::memset(sourceFillPtr, 0, frameSizeBytes);
                        mSource.reset();
                    }
                } else {
                    ::memset(sourceFillPtr, 0, frameSizeBytes);
                }
                soundio_ring_buffer_advance_write_ptr(mOutputRingBuffer, frameSizeBytes);
                ringFrames = soundio_ring_buffer_fill_count(mOutputRingBuffer) / static_cast<int>(sizeof(SampleType));
            }
            auto *ringBuf = reinterpret_cast<SampleType *>(soundio_ring_buffer_read_ptr(mOutputRingBuffer));
            int ringFramesLeft = ringFrames;
            while (ringFramesLeft > 0 && samplesWritten < sampleCount) {
                *reinterpret_cast<SampleType *>(chPtr[0]) = *(ringBuf);
                *reinterpret_cast<SampleType *>(chPtr[1]) = *(ringBuf);
                ringBuf++;
                samplesWritten++;
                chPtr[0] += chStep[0];
                chPtr[1] += chStep[1];
                ringFramesLeft--;
            }
            soundio_ring_buffer_advance_read_ptr(mOutputRingBuffer,
                                                 (ringFrames - ringFramesLeft) * static_cast<int>(sizeof(SampleType)));
        }
        soundio_outstream_end_write(stream);
    } else {
        LOG("SoundIOAudioDevice::sioWriteCallback", "Couldn't lock playback buffer: %s", soundio_strerror(rv));
    }
}

void SoundIOAudioDevice::sioReadCallback(struct SoundIoInStream *stream, int frame_count_min, int frame_count_max) {
    std::lock_guard<std::mutex> sinkLock(mSinkPtrLock);

    // always pull the full input buffer.
    SoundIoChannelArea *bufAreas;
    int sampleCount = frame_count_max;
    auto rv = soundio_instream_begin_read(stream, &bufAreas, &sampleCount);
    if (rv == SoundIoErrorNone) {
        auto *flexPtr = bufAreas[0].ptr;
        int samplesRead = 0;
        int ringFrames;
        while (samplesRead < sampleCount) {
            // fill the ringbuffer as best we can.
            auto *ringWriteBuf = reinterpret_cast<SampleType *>(soundio_ring_buffer_write_ptr(mInputRingBuffer));
            ringFrames = soundio_ring_buffer_free_count(mInputRingBuffer) / static_cast<int>(sizeof(SampleType));
            const int initialRingFrames = ringFrames;
            while ((samplesRead < sampleCount) && (ringFrames > 0)) {
                *(ringWriteBuf++) = *reinterpret_cast<SampleType *>(flexPtr);
                samplesRead++;
                flexPtr += bufAreas[0].step;
                ringFrames--;
            }
            soundio_ring_buffer_advance_write_ptr(mInputRingBuffer,
                                                  (initialRingFrames - ringFrames) *
                                                  static_cast<int>(sizeof(SampleType)));

            // if we have a complete frame, send it to the codec.
            ringFrames = soundio_ring_buffer_fill_count(mInputRingBuffer) / static_cast<int>(sizeof(SampleType));
            while (ringFrames >= frameSizeSamples) {
                auto *sinkFillPtr = reinterpret_cast<SampleType *>(soundio_ring_buffer_read_ptr(mInputRingBuffer));
                if (mSink) {
                    mSink->putAudioFrame(sinkFillPtr);
                }
                soundio_ring_buffer_advance_read_ptr(mInputRingBuffer, frameSizeBytes);
                ringFrames = soundio_ring_buffer_fill_count(mInputRingBuffer) / static_cast<int>(sizeof(SampleType));
            }
        }
        soundio_instream_end_read(stream);
    } else {
        LOG("SoundIOAudioDevice::sioReadCallback", "Couldn't lock recording buffer: %s", soundio_strerror(rv));
    }
}

void SoundIOAudioDevice::close() {
    if (mInputStream) {
        //soundio_instream_pause(mInputStream, true);
        soundio_instream_destroy(mInputStream);
        mInputStream = nullptr;
    }
    if (mOutputStream) {
        //soundio_outstream_pause(mOutputStream, true);
        soundio_outstream_destroy(mOutputStream);
        mOutputStream = nullptr;
    }
}

std::map<AudioDevice::Api, std::string> SoundIOAudioDevice::getAPIs() {
    std::map<AudioDevice::Api, std::string> apiList;

    auto *local_soundIo = soundio_create();
    if (local_soundIo != nullptr) {
        int apiCount = soundio_backend_count(local_soundIo);
        for (int i = 0; i < apiCount; i++) {
            auto backend_num = soundio_get_backend(local_soundIo, i);
            apiList[static_cast<int>(backend_num)] = soundio_backend_name(backend_num);
        }
        soundio_destroy(local_soundIo);
    }
    return std::move(apiList);
}

std::map<int, AudioDevice::DeviceInfo> SoundIOAudioDevice::getCompatibleInputDevicesForApi(AudioDevice::Api api) {
    std::map<int, AudioDevice::DeviceInfo> deviceList;
    auto *local_soundIo = soundio_create();
    if (local_soundIo != nullptr) {
        auto rv = soundio_connect_backend(local_soundIo, static_cast<enum SoundIoBackend>(api));
        if (rv == SoundIoErrorNone) {
            soundio_force_device_scan(local_soundIo);
            soundio_flush_events(local_soundIo);
            int device_count = soundio_input_device_count(local_soundIo);
            for (int i = 0; i < device_count; i++) {
                auto *device_info = soundio_get_input_device(local_soundIo, i);
                if (device_info != nullptr) {
                    // check the device parameters...

                    // eliminate raw devices.  (They cause us more grief than it's worth).
                    if (device_info->is_raw) {
                        continue;
                    }
                    if (isAbleToOpen(device_info)) {
                        LOG("SoundIOAudioDevice", "input device %s - OK", device_info->name);
                        deviceList.emplace(i, DeviceInfo(device_info->name, device_info->id));
                    }
                }
            }
        } else {
            LOG("SoundIOAudioDevice::getCompatibleInputDevicesForApi", "Couldn't open API: %s", soundio_strerror(rv));
        }
        soundio_destroy(local_soundIo);
    }
    return std::move(deviceList);
}

std::map<int, AudioDevice::DeviceInfo> SoundIOAudioDevice::getCompatibleOutputDevicesForApi(AudioDevice::Api api) {
    std::map<int, DeviceInfo> deviceList;
    auto *local_soundIo = soundio_create();
    if (local_soundIo != nullptr) {
        auto rv = soundio_connect_backend(local_soundIo, static_cast<enum SoundIoBackend>(api));
        if (rv == SoundIoErrorNone) {
            soundio_force_device_scan(local_soundIo);
            soundio_flush_events(local_soundIo);
            int device_count = soundio_output_device_count(local_soundIo);
            for (unsigned i = 0; i < device_count; i++) {
                auto *device_info = soundio_get_output_device(local_soundIo, i);
                if (device_info != nullptr) {
                    // check the device parameters...

                    // eliminate raw devices.  (They cause us more grief than it's worth).
                    if (device_info->is_raw) {
                        continue;
                    }
                    if (isAbleToOpen(device_info, true)) {
                        LOG("SoundIOAudioDevice", "output device %s - OK", device_info->name);
                        deviceList.emplace(i, DeviceInfo(device_info->name, device_info->id));
                    }
                }
            }
        } else {
            LOG("SoundIOAudioDevice::getCompatibleOutputDevicesForApi", "Couldn't open API: %s", soundio_strerror(rv));
        }
        soundio_destroy(local_soundIo);
    }
    return std::move(deviceList);
}

bool SoundIOAudioDevice::isAbleToOpen(SoundIoDevice *device_info, bool for_output) {
    // first, format.
    if (!soundio_device_supports_format(device_info, SoundIoFormatFloat32NE)) {
        LOG("SoundIOAudioDevice", "device %s - can't handle float pcm.", device_info->name);
        return false;
    }

    // next, samplerate.
    if (!soundio_device_supports_sample_rate(device_info, sampleRateHz)) {
        LOG("SoundIOAudioDevice", "device %s - can't handle sampling rate.", device_info->name);
        return false;
    }

    // next channel layouts.
    auto *monoLayout = soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdMono);
    auto *stereoLayout = soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdStereo);
    if (!soundio_device_supports_layout(device_info, monoLayout) &&
        !(for_output && soundio_device_supports_layout(device_info, stereoLayout))) {
        if (for_output) {
            LOG("SoundIOAudioDevice", "device %s - doesn't support monaural or stereo audio", device_info->name);
        } else {
            LOG("SoundIOAudioDevice", "device %s - doesn't support monaural audio", device_info->name);
        }
        return false;
    }
    return true;
}

SoundIoDevice *SoundIOAudioDevice::getInputDeviceForId(const std::string &deviceId) {
    soundio_flush_events(mSoundIO);
    auto device_count = soundio_input_device_count(mSoundIO);
    for (auto i = 0; i < device_count; i++) {
        auto *device = soundio_get_input_device(mSoundIO, i);
        if (deviceId == device->id) {
            return device;
        }
    }
    return nullptr;
}

SoundIoDevice *SoundIOAudioDevice::getOutputDeviceForId(const std::string &deviceId) {
    soundio_flush_events(mSoundIO);
    auto device_count = soundio_output_device_count(mSoundIO);
    for (auto i = 0; i < device_count; i++) {
        auto *device = soundio_get_output_device(mSoundIO, i);
        if (deviceId == device->id) {
            return device;
        }
    }
    return nullptr;
}

size_t SoundIOAudioDevice::optimumFrameCount(size_t staleframes, size_t min, size_t max) {
    size_t frameCount;
    if (staleframes > 0 && staleframes > min) {
        frameCount = staleframes;
    } else {
        frameCount = std::max<size_t>(staleframes + frameSizeSamples, min);
    }
    frameCount = std::min<size_t>(frameCount, max);
    if (frameCount == 0) {
        frameCount = std::min<size_t>(frameSizeSamples, max);
    }
    return frameCount;
}

void SoundIOAudioDevice::staticSioReadCallback(struct SoundIoInStream *stream, int frame_count_min, int frame_count_max) {
    auto *thisAd = reinterpret_cast<SoundIOAudioDevice *>(stream->userdata);
    thisAd->sioReadCallback(stream, frame_count_min, frame_count_max);
}

void SoundIOAudioDevice::staticSioWriteCallback(struct SoundIoOutStream *stream, int frame_count_min, int frame_count_max) {
    auto *thisAd = reinterpret_cast<SoundIOAudioDevice *>(stream->userdata);
    thisAd->sioWriteCallback(stream, frame_count_min, frame_count_max);
}

void
SoundIOAudioDevice::staticSioOutputUnderflowCallback(struct SoundIoOutStream *stream)
{
#ifndef NDEBUG
    LOG("SoundIOAudioDevice::Output", "Output Underflowed");
#endif
    auto *thisAd = reinterpret_cast<SoundIOAudioDevice *>(stream->userdata);
    thisAd->OutputUnderflows.fetch_add(1);
}

void
SoundIOAudioDevice::staticSioOutputErrorCallback(struct SoundIoOutStream *stream, int err)
{
    LOG("SoundIOAudioDevice::Output", "Got Error: %s", soundio_strerror(err));
}

void
SoundIOAudioDevice::staticSioInputOverflowCallback(struct SoundIoInStream *stream)
{
#ifndef NDEBUG
    LOG("SoundIOAudioDevice::Input", "Input Overflowed");
#endif
    auto *thisAd = reinterpret_cast<SoundIOAudioDevice *>(stream->userdata);
    thisAd->InputOverflows.fetch_add(1);
}

void
SoundIOAudioDevice::staticSioInputErrorCallback(struct SoundIoInStream *stream, int err)
{
    LOG("SoundIOAudioDevice::Input", "Got Error: %s", soundio_strerror(err));
}



/* ========== Factory hooks ============= */
map<AudioDevice::Api, std::string> AudioDevice::getAPIs() {
    return SoundIOAudioDevice::getAPIs();
}

map<int, AudioDevice::DeviceInfo> AudioDevice::getCompatibleInputDevicesForApi(AudioDevice::Api api) {
    return SoundIOAudioDevice::getCompatibleInputDevicesForApi(api);
}

map<int, AudioDevice::DeviceInfo> AudioDevice::getCompatibleOutputDevicesForApi(AudioDevice::Api api) {
    return SoundIOAudioDevice::getCompatibleOutputDevicesForApi(api);
}

std::shared_ptr<AudioDevice>
AudioDevice::makeDevice(
        const std::string &userStreamName,
        const std::string &outputDeviceId,
        const std::string &inputDeviceId,
        AudioDevice::Api audioApi) {
    auto devsp = std::make_shared<SoundIOAudioDevice>(userStreamName, outputDeviceId, inputDeviceId, audioApi);
    return devsp;
}
