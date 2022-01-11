cmake_minimum_required(VERSION 3.10)
project(afv_native LANGUAGES CXX)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
	if(NOT DEFINED CONAN_LINK_RUNTIME AND DEFINED CONAN_SETTINGS_COMPILER_RUNTIME)
		message(WARNING "Forcing MSVC runtime based on conan profile")
		# this means Conan is not running the build, so we need to fix our linker flags ourself.... although we can probably cheat...
		set(CONAN_LINK_RUNTIME "/${CONAN_SETTINGS_COMPILER_RUNTIME}")
		conan_set_vs_runtime()
	endif()
endif(MSVC)

set(CMAKE_CXX_STANDARD 17)

set(AFV_NATIVE_HEADERS
		include/afv-native/Client.h
		include/afv-native/event.h
		include/afv-native/Log.h
		include/afv-native/afv/APISession.h
		include/afv-native/afv/EffectResources.h
		include/afv-native/afv/params.h
		include/afv-native/afv/RadioSimulation.h
		include/afv-native/afv/RemoteVoiceSource.h
		include/afv-native/afv/RollingAverage.h
		include/afv-native/afv/VoiceCompressionSink.h
		include/afv-native/afv/VoiceSession.h
		include/afv-native/afv/dto/AuthRequest.h
		include/afv-native/afv/dto/PostCallsignResponse.h
		include/afv-native/afv/dto/Station.h
		include/afv-native/afv/dto/Transceiver.h
		include/afv-native/afv/dto/VoiceServerConnectionData.h
		include/afv-native/afv/dto/interfaces/IAudio.h
		include/afv-native/afv/dto/interfaces/IAudioDirect.h
		include/afv-native/afv/dto/interfaces/IAudioVoiceRoom.h
		include/afv-native/afv/dto/domain/RxTransceiver.h
		include/afv-native/afv/dto/domain/TxTransceiver.h
		include/afv-native/afv/dto/voice_server/AudioOnDirect.h
		include/afv-native/afv/dto/voice_server/AudioRxOnTransceivers.h
		include/afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h
		include/afv-native/afv/dto/voice_server/Heartbeat.h
		include/afv-native/audio/audio_params.h
		include/afv-native/audio/AudioDevice.h
		include/afv-native/audio/BiQuadFilter.h
		include/afv-native/audio/FilterSource.h
		include/afv-native/audio/IFilter.h
		include/afv-native/audio/ISampleSink.h
		include/afv-native/audio/ISampleSource.h
		include/afv-native/audio/ISampleStorage.h
		include/afv-native/audio/OutputMixer.h
		include/afv-native/audio/PinkNoiseGenerator.h
		include/afv-native/audio/RecordedSampleSource.h
		include/afv-native/audio/SineToneSource.h
		include/afv-native/audio/SinkFrameSizeAdjuster.h
		include/afv-native/audio/SourceFrameSizeAdjuster.h
		include/afv-native/audio/SimpleCompressorEffect.h
		include/afv-native/audio/SpeexPreprocessor.h
		include/afv-native/audio/VHFFilterSource.h
		include/afv-native/audio/WavFile.h
		include/afv-native/audio/WavSampleStorage.h
		include/afv-native/audio/WhiteNoiseGenerator.h
		include/afv-native/cryptodto/Channel.h
		include/afv-native/cryptodto/dto/ICryptoDTO.h
		include/afv-native/cryptodto/params.h
		include/afv-native/cryptodto/SequenceTest.h
		include/afv-native/cryptodto/UDPChannel.h
		include/afv-native/cryptodto/dto/ChannelConfig.h
		include/afv-native/cryptodto/dto/Header.h
		include/afv-native/event/EventTimer.h
		include/afv-native/event/EventCallbackTimer.h
		include/afv-native/http/EventTransferManager.h
		include/afv-native/http/http.h
		include/afv-native/http/Request.h
		include/afv-native/http/RESTRequest.h
		include/afv-native/http/TransferManager.h
		include/afv-native/util/base64.h
		include/afv-native/util/ChainedCallback.h
		include/afv-native/util/monotime.h
		include/afv-native/utility.h
		)
add_library(afv_native SHARED
		${AFV_NATIVE_HEADERS}
		src/afv/APISession.cpp
		src/afv/EffectResources.cpp
		src/afv/RadioSimulation.cpp
		src/afv/RemoteVoiceSource.cpp
		src/afv/VoiceCompressionSink.cpp
		src/afv/VoiceSession.cpp
		src/afv/dto/AuthRequest.cpp
		src/afv/dto/PostCallsignResponse.cpp
		src/afv/dto/Station.cpp
		src/afv/dto/Transceiver.cpp
		src/afv/dto/VoiceServerConnectionData.cpp
		src/audio/AudioDevice.cpp
		src/audio/FilterSource.cpp
		src/audio/BiQuadFilter.cpp
		src/audio/OutputMixer.cpp
		src/audio/RecordedSampleSource.cpp
		src/audio/SineToneSource.cpp
		src/audio/SinkFrameSizeAdjuster.cpp
		src/audio/SourceFrameSizeAdjuster.cpp
		src/audio/SpeexPreprocessor.cpp
		src/audio/WavFile.cpp
		src/audio/WavSampleStorage.cpp
		src/core/Client.cpp
		src/core/Log.cpp
		src/cryptodto/Channel.cpp
		src/cryptodto/SequenceTest.cpp
		src/cryptodto/UDPChannel.cpp
		src/cryptodto/dto/ChannelConfig.cpp
		src/cryptodto/dto/Header.cpp
		src/event/EventCallbackTimer.cpp
		src/event/EventTimer.cpp
		src/http/EventTransferManager.cpp
		src/http/TransferManager.cpp
		src/http/Request.cpp
		src/http/RESTRequest.cpp
		src/util/base64.cpp
		src/util/monotime.cpp
		src/audio/PortAudioAudioDevice.cpp
		src/audio/SimpleCompressorEffect.cpp
		src/audio/PortAudioAudioDevice.h
		extern/simpleSource/SimpleComp.cpp
		extern/simpleSource/SimpleComp.h
		extern/simpleSource/SimpleEnvelope.cpp
		extern/simpleSource/SimpleEnvelope.h
		extern/simpleSource/SimpleGain.h
		extern/simpleSource/SimpleGate.cpp
		extern/simpleSource/SimpleGate.h
		extern/simpleSource/SimpleHeader.h
		extern/simpleSource/SimpleLimit.cpp
		extern/simpleSource/SimpleLimit.h
		src/audio/VHFFilterSource.cpp
		src/NativeWrapper.cpp)
		
# organise sources in generated projects.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source" FILES ${AFV_NATIVE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/afv-native PREFIX "Headers" FILES ${AFV_NATIVE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/extern PREFIX "Third Party" FILES ${AFV_NATIVE_THIRDPARTY_SOURCES})

if(WIN32)
	set(OPUS_URL https://github.com/xpilot-project/opus/releases/download/xpilot/windows-opus.lib)
	set(OPUS_LIBARY_PATH ${CMAKE_BINARY_DIR}/import/windows-opus.lib)
	
	set(PORTAUDIO_URL https://github.com/xpilot-project/portaudio/releases/download/xpilot/windows-portaudio_static_x64.lib)
	set(PORTAUDIO_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/windows-portaudio_static_x64.lib)
		
	set(SPEEXDSP_URL https://github.com/xpilot-project/speexdsp/releases/download/xpilot/windows-speexdsp.lib)
	set(SPEEXDSP_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/windows-speexdsp.lib)

	file(DOWNLOAD ${OPUS_URL} ${OPUS_LIBARY_PATH})
	file(DOWNLOAD ${PORTAUDIO_URL} ${PORTAUDIO_LIBRARY_PATH})
	file(DOWNLOAD ${SPEEXDSP_URL} ${SPEEXDSP_LIBRARY_PATH})
endif()

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

target_link_libraries(afv_native
	PUBLIC
	CONAN_PKG::openssl
	CONAN_PKG::msgpack
	CONAN_PKG::nlohmann_json
	CONAN_PKG::libcurl
	CONAN_PKG::libevent
	${OPUS_LIBARY_PATH}
	${PORTAUDIO_LIBRARY_PATH}
	${SPEEXDSP_LIBRARY_PATH}
	${CORE_AUDIO}
	${AUDIO_TOOLBOX}
	${AUDIO_UNIT}
	${CORE_FOUNDATION}
	${CORE_SERVICES})
else()
target_link_libraries(afv_native
	PUBLIC
	CONAN_PKG::openssl
	CONAN_PKG::msgpack
	CONAN_PKG::nlohmann_json
	CONAN_PKG::libcurl
	CONAN_PKG::libevent
	${OPUS_LIBARY_PATH}
	${PORTAUDIO_LIBRARY_PATH}
	${SPEEXDSP_LIBRARY_PATH})
endif()

target_include_directories(afv_native
		PRIVATE
		${CMAKE_SOURCE_DIR}/extern/cpp-jwt/include
		${CMAKE_SOURCE_DIR}/extern/simpleSource
		PUBLIC
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/extern)

if(MSVC)
	# I hate to do this this way, but we must force MSVC to define the standard math macros whereever the afv headers
	# are used
	target_compile_definitions(afv_native PUBLIC _USE_MATH_DEFINES)
endif()

install(TARGETS afv_native
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include" # source directory
		DESTINATION "." # target directory
		FILES_MATCHING # install only matched files
		PATTERN "*.h" # select header files
		)

set_target_properties(afv_native PROPERTIES OUTPUT_NAME "afv")