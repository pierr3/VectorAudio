cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(vector_audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions(-DVECTOR_VERSION="1.3.1a")
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT=1)

add_subdirectory(extern/afv-native)

if(APPLE)
    execute_process(
        COMMAND arch
        RESULT_VARIABLE VECTOR_ARCH_CMD
        OUTPUT_VARIABLE VECTOR_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (VECTOR_ARCH_CMD EQUAL 0)
        add_definitions(-DVECTOR_ARCH="${VECTOR_ARCH}")
    endif()

    # Suppress warnings of the deprecation of glut functions on macOS.
    add_definitions(-Wno-deprecated-declarations -Wno-many-braces-around-scalar-init)
endif()

if (WIN32)
    add_definitions(-DCMAKE_CONFIGURATION_TYPES=Release)
endif()

option(SFML_BUILD_AUDIO "Build audio" OFF)
option(SFML_BUILD_NETWORK "Build network" OFF)

find_package(OpenGL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(httplib REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-http-parser REQUIRED)
find_package(restinio CONFIG REQUIRED)
find_path(NEARGYE_SEMVER_INCLUDE_DIRS "neargye/semver.hpp")
find_package(Threads REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)

set(APPLE_EXTRA_LIBS "")

include_directories(    ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/extern/PlatformFolders/sago
                        ${OPENGL_INCLUDE_DIR}
                        ${NEARGYE_SEMVER_INCLUDE_DIRS})

if (APPLE)
    set(APPLE_EXTRA_LIBS src/osx_resources.mm src/window_manager.mm)
endif()


add_executable(vector_audio src/main.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui-SFML.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_stdlib.cpp
                src/application.cpp
                src/config.cpp
                src/updater.cpp
                src/window_manager.cpp
                src/data_file_handler.cpp
                src/modals/settings.cpp
                src/single_instance.cpp
                ${CMAKE_SOURCE_DIR}/extern/PlatformFolders/sago/platform_folders.cpp
                ${APPLE_EXTRA_LIBS})

if (WIN32)
    set_target_properties(vector_audio PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS")
endif()

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CORE_SERVICES CoreServices)
	find_library(CORE_AUDIO CoreAudio)
	find_library(AUDIO_TOOLBOX AudioToolbox)
	find_library(AUDIO_UNIT AudioUnit)
	find_library(CORE_FOUNDATION CoreFoundation)
	find_library(CORE_SERVICES CoreServices)

	target_link_libraries(vector_audio
		PRIVATE
        ${COCOA_LIBRARY}
		${CORE_AUDIO}
		${AUDIO_TOOLBOX}
		${AUDIO_UNIT}
		${CORE_FOUNDATION}
		${CORE_SERVICES})
endif()

target_link_libraries(vector_audio
    PRIVATE
    OpenSSL::SSL OpenSSL::Crypto 
    sfml-system sfml-window sfml-graphics
    toml11::toml11
    nlohmann_json nlohmann_json::nlohmann_json
    restinio::restinio
    afv_native
    httplib::httplib
    Threads::Threads
    ${OPENGL_LIBRARY})

if (WIN32)
    add_custom_command(TARGET vector_audio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:vector_audio> $<TARGET_FILE_DIR:vector_audio>
    COMMAND_EXPAND_LISTS)
endif()

