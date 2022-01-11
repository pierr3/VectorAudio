cmake_minimum_required(VERSION 3.10)
project(afv_unix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/extern/afv-native/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(    ${SDL2_INCLUDE_DIRS}
                        ${OPENGL_INCLUDE_DIRS}
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/extern/afv-native/include
                        ${CMAKE_SOURCE_DIR}/extern/afv-native/extern
                        ${CMAKE_SOURCE_DIR}/extern/afv-native/extern/cpp-jwt/include
                        ${CMAKE_SOURCE_DIR}/extern/afv-native/extern/simpleSource)



add_executable(afv_unix 
                src/main.cpp
                src/application.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_impl_opengl3.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_impl_sdl.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp)

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
        target_link_libraries(afv_unix ${CMAKE_SOURCE_DIR}/lib/libafv-arm64.a)

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