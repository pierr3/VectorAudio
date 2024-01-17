cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(vector_audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT=1)

# Clang preferences

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)


# Read the VERSION file
file(READ "VERSION" VERSION_CONTENT)
string(REGEX MATCHALL "[0-9]+" VERSION_NUMBERS "${VERSION_CONTENT}")
list(LENGTH VERSION_NUMBERS NUM_VERSIONS)
if (NUM_VERSIONS LESS 3)
    message(FATAL_ERROR "Invalid VERSION file. It should contain major, minor, and patch version numbers.")
endif()

# Extract major, minor, and patch version numbers from the list
list(GET VERSION_NUMBERS 0 version_major)
list(GET VERSION_NUMBERS 1 version_minor)
list(GET VERSION_NUMBERS 2 version_patch)

# Set version variables
set(VERSION_MAJOR ${version_major})
set(VERSION_MINOR ${version_minor})
set(VERSION_PATCH ${version_patch})

add_definitions(-DVECTOR_VERSION="${VERSION_CONTENT}")
configure_file(vector_audio.rc.in ${CMAKE_SOURCE_DIR}/vector_audio.rc @ONLY)

include_directories(lib/include)

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
find_package(absl CONFIG REQUIRED)
find_package(SFML COMPONENTS system window graphics audio CONFIG REQUIRED)

set(APPLE_EXTRA_LIBS "")

include_directories(    ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/extern/PlatformFolders/sago
                        ${OPENGL_INCLUDE_DIR}
                        ${NEARGYE_SEMVER_INCLUDE_DIRS})

if (APPLE)
    set(APPLE_EXTRA_LIBS src/native/osx_resources.mm src/native/window_manager.mm)
endif()

if(WIN32)
    set(GUI_TYPE WIN32)
elseif(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif()

add_executable(vector_audio ${GUI_TYPE} src/main.cpp
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
                src/native/window_manager.cpp
                src/data_file_handler.cpp
                src/ui/modals/settings.cpp
                src/native/single_instance.cpp
                src/sdk/sdk.cpp
                src/native/win32_key_util.cpp
                ${CMAKE_SOURCE_DIR}/extern/PlatformFolders/sago/platform_folders.cpp
                ${APPLE_EXTRA_LIBS}
                vector_audio.rc)


if (WIN32)
    set_target_properties(vector_audio PROPERTIES
        WIN32_EXECUTABLE YES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

    #afv
    find_library(LIB_AFV afv_native PATHS ${CMAKE_SOURCE_DIR}/lib/windows/ NO_DEFAULT_PATH)
    if(NOT LIB_AFV)
        message(FATAL_ERROR "libafv library not found")
    endif()
    message(STATUS "libafv: ${LIB_AFV}")
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
    
    #afv
    find_library(LIB_AFV afv_native PATHS ${CMAKE_SOURCE_DIR}/lib/macos/ NO_DEFAULT_PATH)
    if(NOT LIB_AFV)
        message(FATAL_ERROR "libafv library not found")
    endif()
    message(STATUS "libafv: ${LIB_AFV}")
endif()

if (UNIX AND NOT APPLE)
    #afv
    find_library(LIB_AFV afv_native PATHS ${CMAKE_SOURCE_DIR}/lib/linux/ NO_DEFAULT_PATH)
    if(NOT LIB_AFV)
        message(FATAL_ERROR "libafv library not found")
    endif()
    message(STATUS "libafv: ${LIB_AFV}")
endif()

target_link_libraries(vector_audio
    PRIVATE
    OpenSSL::SSL OpenSSL::Crypto 
    sfml-system sfml-window sfml-graphics sfml-audio
    toml11::toml11
    ${LIB_AFV}
    nlohmann_json nlohmann_json::nlohmann_json
    restinio::restinio
    httplib::httplib
    Threads::Threads
    absl::strings
    ${OPENGL_LIBRARY})

if (WIN32)
    add_custom_command(TARGET vector_audio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:vector_audio> $<TARGET_FILE_DIR:vector_audio>
    COMMAND_EXPAND_LISTS)
endif()

