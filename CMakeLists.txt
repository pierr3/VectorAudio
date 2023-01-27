cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(vector_audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions(-DVECTOR_VERSION="0.5.0")

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

add_subdirectory(${PROJECT_SOURCE_DIR}/extern/afv-native)


find_package(OpenGL REQUIRED)
find_path(CPP_HTTPLIB_INCLUDE_DIRS "httplib.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SFML COMPONENTS system window graphics network CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-http-parser REQUIRED)
find_package(restinio CONFIG REQUIRED)
find_path(NEARGYE_SEMVER_INCLUDE_DIRS "neargye/semver.hpp")

set(APPLE_EXTRA_LIBS "")

include_directories(    ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/extern/afv-native/include/afv-native
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/extern/PlatformFolders/sago
                        ${CPP_HTTPLIB_INCLUDE_DIRS}
                        ${OPENGL_INCLUDE_DIR}
                        ${NEARGYE_SEMVER_INCLUDE_DIRS})

if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

if (APPLE)
    set(APPLE_EXTRA_LIBS src/osx_resources.mm)
endif()

if (WIN32)
    find_library(AFV_LIB speexdsp.lib)
endif()

add_executable(vector_audio src/main.cpp
                src/application.cpp
                src/config.cpp
                src/updater.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui-SFML.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_stdlib.cpp
                ${CMAKE_SOURCE_DIR}/extern/PlatformFolders/sago/platform_folders.cpp
                ${APPLE_EXTRA_LIBS})

if (WIN32)
    set_target_properties(vector_audio PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

target_link_libraries(vector_audio
    OpenSSL::SSL OpenSSL::Crypto 
    sfml-system sfml-window sfml-graphics
    toml11::toml11
    nlohmann_json nlohmann_json::nlohmann_json
    fmt::fmt-header-only
    restinio::restinio
    afv_native
    ${OPENGL_LIBRARY} ${AFV_LIB} ${CMAKE_THREAD_LIBS_INIT})

if (WIN32)
    add_custom_command(TARGET vector_audio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:vector_audio> $<TARGET_FILE_DIR:vector_audio>
    COMMAND_EXPAND_LISTS)
endif()

