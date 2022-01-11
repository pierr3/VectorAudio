cmake_minimum_required(VERSION 3.10)
project(afv_unix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(AFV_NATIVE_HEADERS
afv-native/include/afv-native/Client.h
afv-native/include/afv-native/event.h
afv-native/include/afv-native/Log.h
afv-native/include/afv-native/afv/APISession.h
afv-native/include/afv-native/afv/EffectResources.h
afv-native/include/afv-native/afv/params.h
afv-native/include/afv-native/afv/RadioSimulation.h
afv-native/include/afv-native/afv/RemoteVoiceSource.h
afv-native/include/afv-native/afv/RollingAverage.h
afv-native/include/afv-native/afv/VoiceCompressionSink.h
afv-native/include/afv-native/afv/VoiceSession.h
afv-native/include/afv-native/afv/dto/AuthRequest.h
afv-native/include/afv-native/afv/dto/PostCallsignResponse.h
afv-native/include/afv-native/afv/dto/Station.h
afv-native/include/afv-native/afv/dto/Transceiver.h
afv-native/include/afv-native/afv/dto/VoiceServerConnectionData.h
afv-native/include/afv-native/afv/dto/interfaces/IAudio.h
afv-native/include/afv-native/afv/dto/interfaces/IAudioDirect.h
afv-native/include/afv-native/afv/dto/interfaces/IAudioVoiceRoom.h
afv-native/include/afv-native/afv/dto/domain/RxTransceiver.h
afv-native/include/afv-native/afv/dto/domain/TxTransceiver.h
afv-native/include/afv-native/afv/dto/voice_server/AudioOnDirect.h
afv-native/include/afv-native/afv/dto/voice_server/AudioRxOnTransceivers.h
afv-native/include/afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h
afv-native/include/afv-native/afv/dto/voice_server/Heartbeat.h
afv-native/include/afv-native/audio/audio_params.h
afv-native/include/afv-native/audio/AudioDevice.h
afv-native/include/afv-native/audio/BiQuadFilter.h
afv-native/include/afv-native/audio/FilterSource.h
afv-native/include/afv-native/audio/IFilter.h
afv-native/include/afv-native/audio/ISampleSink.h
afv-native/include/afv-native/audio/ISampleSource.h
afv-native/include/afv-native/audio/ISampleStorage.h
afv-native/include/afv-native/audio/OutputMixer.h
afv-native/include/afv-native/audio/PinkNoiseGenerator.h
afv-native/include/afv-native/audio/RecordedSampleSource.h
afv-native/include/afv-native/audio/SineToneSource.h
afv-native/include/afv-native/audio/SinkFrameSizeAdjuster.h
afv-native/include/afv-native/audio/SourceFrameSizeAdjuster.h
afv-native/include/afv-native/audio/SimpleCompressorEffect.h
afv-native/include/afv-native/audio/SpeexPreprocessor.h
afv-native/include/afv-native/audio/VHFFilterSource.h
afv-native/include/afv-native/audio/WavFile.h
afv-native/include/afv-native/audio/WavSampleStorage.h
afv-native/include/afv-native/audio/WhiteNoiseGenerator.h
afv-native/include/afv-native/cryptodto/Channel.h
afv-native/include/afv-native/cryptodto/dto/ICryptoDTO.h
afv-native/include/afv-native/cryptodto/params.h
afv-native/include/afv-native/cryptodto/SequenceTest.h
afv-native/include/afv-native/cryptodto/UDPChannel.h
afv-native/include/afv-native/cryptodto/dto/ChannelConfig.h
afv-native/include/afv-native/cryptodto/dto/Header.h
afv-native/include/afv-native/event/EventTimer.h
afv-native/include/afv-native/event/EventCallbackTimer.h
afv-native/include/afv-native/http/EventTransferManager.h
afv-native/include/afv-native/http/http.h
afv-native/include/afv-native/http/Request.h
afv-native/include/afv-native/http/RESTRequest.h
afv-native/include/afv-native/http/TransferManager.h
afv-native/include/afv-native/util/base64.h
afv-native/include/afv-native/util/ChainedCallback.h
afv-native/include/afv-native/util/monotime.h
afv-native/include/afv-native/utility.h)

set(AFV_NATIVE_AUDIO_SOURCES
    afv-native/src/audio/PortAudioAudioDevice.cpp
    afv-native/src/audio/PortAudioAudioDevice.h)

set(AFV_NATIVE_SOURCES
    afv-native/src/afv/APISession.cpp
    afv-native/src/afv/EffectResources.cpp
    afv-native/src/afv/RadioSimulation.cpp
    afv-native/src/afv/RemoteVoiceSource.cpp
    afv-native/src/afv/VoiceCompressionSink.cpp
    afv-native/src/afv/VoiceSession.cpp
    afv-native/src/afv/dto/AuthRequest.cpp
    afv-native/src/afv/dto/PostCallsignResponse.cpp
    afv-native/src/afv/dto/Station.cpp
    afv-native/src/afv/dto/Transceiver.cpp
    afv-native/src/afv/dto/VoiceServerConnectionData.cpp
    afv-native/src/audio/AudioDevice.cpp
    afv-native/src/audio/FilterSource.cpp
    afv-native/src/audio/BiQuadFilter.cpp
    afv-native/src/audio/OutputMixer.cpp
    afv-native/src/audio/RecordedSampleSource.cpp
    afv-native/src/audio/SineToneSource.cpp
    afv-native/src/audio/SinkFrameSizeAdjuster.cpp
    afv-native/src/audio/SourceFrameSizeAdjuster.cpp
    afv-native/src/audio/SpeexPreprocessor.cpp
    afv-native/src/audio/WavFile.cpp
    afv-native/src/audio/WavSampleStorage.cpp
    afv-native/src/core/Client.cpp
    afv-native/src/core/Log.cpp
    afv-native/src/cryptodto/Channel.cpp
    afv-native/src/cryptodto/SequenceTest.cpp
    afv-native/src/cryptodto/UDPChannel.cpp
    afv-native/src/cryptodto/dto/ChannelConfig.cpp
    afv-native/src/cryptodto/dto/Header.cpp
    afv-native/src/event/EventCallbackTimer.cpp
    afv-native/src/event/EventTimer.cpp
    afv-native/src/http/EventTransferManager.cpp
    afv-native/src/http/TransferManager.cpp
    afv-native/src/http/Request.cpp
    afv-native/src/http/RESTRequest.cpp
    afv-native/src/util/base64.cpp
    afv-native/src/util/monotime.cpp
    afv-native/src/audio/PortAudioAudioDevice.cpp
    afv-native/src/audio/SimpleCompressorEffect.cpp
    afv-native/src/audio/PortAudioAudioDevice.h
    afv-native/extern/simpleSource/SimpleComp.cpp
    afv-native/extern/simpleSource/SimpleComp.h
    afv-native/extern/simpleSource/SimpleEnvelope.cpp
    afv-native/extern/simpleSource/SimpleEnvelope.h
    afv-native/extern/simpleSource/SimpleGain.h
    afv-native/extern/simpleSource/SimpleGate.cpp
    afv-native/extern/simpleSource/SimpleGate.h
    afv-native/extern/simpleSource/SimpleHeader.h
    afv-native/extern/simpleSource/SimpleLimit.cpp
    afv-native/extern/simpleSource/SimpleLimit.h
    afv-native/src/audio/VHFFilterSource.cpp
    afv-native/src/NativeWrapper.cpp
    ${AFV_NATIVE_AUDIO_SOURCES})

include_directories(    ${SDL2_INCLUDE_DIRS}
                        ${OPENGL_INCLUDE_DIRS}
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/afv-native/extern/cpp-jwt/include
                        ${CMAKE_SOURCE_DIR}/afv-native/extern/simpleSource
                        ${CMAKE_SOURCE_DIR}/afv-native/extern
                        ${CMAKE_SOURCE_DIR}/afv-native/include
                        ${CMAKE_SOURCE_DIR}/extern)



add_executable(afv_unix src/main.cpp
                src/application.cpp
                src/config.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_impl_opengl3.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_impl_sdl.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_stdlib.cpp
                ${AFV_NATIVE_HEADERS}
                ${AFV_NATIVE_SOURCES}
                ${AFV_API_SOURCES})

target_link_libraries(afv_unix ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} CONAN_PKG::openssl
CONAN_PKG::msgpack
CONAN_PKG::nlohmann_json
CONAN_PKG::libcurl
CONAN_PKG::libevent)

if(APPLE)
	execute_process(
			COMMAND brew --prefix opus
			RESULT_VARIABLE BREW_OPUS
			OUTPUT_VARIABLE BREW_OPUS_PREFIX
			OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (BREW_OPUS EQUAL 0 AND EXISTS "${BREW_OPUS_PREFIX}")
        message(STATUS "Found Opus keg installed by Homebrew at ${BREW_OPUS_PREFIX}")
        set(OPUS_LIBARY_PATH "${BREW_OPUS_PREFIX}/lib/libopus.a")
    endif()

	execute_process(
			COMMAND brew --prefix portaudio
			RESULT_VARIABLE BREW_PORTAUDIO
			OUTPUT_VARIABLE BREW_PORTAUDIO_PREFIX
			OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (BREW_PORTAUDIO EQUAL 0 AND EXISTS "${BREW_PORTAUDIO_PREFIX}")
        message(STATUS "Found PortAudio keg installed by Homebrew at ${BREW_PORTAUDIO_PREFIX}")
        set(PORTAUDIO_LIBRARY_PATH "${BREW_PORTAUDIO_PREFIX}/lib/libportaudio.a")
    endif()
		
	execute_process(
			COMMAND brew --prefix speexdsp
			RESULT_VARIABLE BREW_SPEEX
			OUTPUT_VARIABLE BREW_SPEEX_PREFIX
			OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (BREW_SPEEX EQUAL 0 AND EXISTS "${BREW_SPEEX_PREFIX}")
        message(STATUS "Found SpeexDSP keg installed by Homebrew at ${BREW_SPEEX_PREFIX}")
        set(SPEEXDSP_LIBRARY_PATH "${BREW_SPEEX_PREFIX}/lib/libspeexdsp.a")
		set(SPEEXDSP_LIBRARY_INCLUDE "${BREW_SPEEX_PREFIX}/include")
    endif()
endif()

if(APPLE)
        find_library(CORE_AUDIO CoreAudio)
        find_library(AUDIO_TOOLBOX AudioToolbox)
        find_library(AUDIO_UNIT AudioUnit)
        find_library(CORE_FOUNDATION CoreFoundation)
        find_library(CORE_SERVICES CoreServices)

        target_link_libraries(afv_unix
                ${OPUS_LIBARY_PATH}
                ${PORTAUDIO_LIBRARY_PATH}
                ${SPEEXDSP_LIBRARY_PATH}
                ${CORE_AUDIO}
                ${AUDIO_TOOLBOX}
                ${AUDIO_UNIT}
                ${CORE_FOUNDATION}
                ${CORE_SERVICES})
else()
        target_link_libraries(afv_unix
            ${OPUS_LIBARY_PATH}
            ${PORTAUDIO_LIBRARY_PATH}
            ${SPEEXDSP_LIBRARY_PATH})
endif()