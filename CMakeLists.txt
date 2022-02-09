cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(vector_audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

execute_process(
			COMMAND git -C ${PROJECT_SOURCE_DIR} rev-parse --short HEAD
			RESULT_VARIABLE VECTOR_GIT_VERSION_CMD
			OUTPUT_VARIABLE VECTOR_GIT_VERSION
			OUTPUT_STRIP_TRAILING_WHITESPACE)

if (VECTOR_GIT_VERSION_CMD EQUAL 0)
    add_definitions(-DVECTOR_VERSION="${VECTOR_GIT_VERSION}")
else()
    add_definitions(-DVECTOR_VERSION="dev")
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/extern/afv-native)

find_package(OpenGL REQUIRED)
find_path(CPP_HTTPLIB_INCLUDE_DIRS "httplib.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SFML COMPONENTS system window graphics network CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

set(APPLE_EXTRA_LIBS "")

include_directories(    ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/extern/afv-native/include/afv-native
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CPP_HTTPLIB_INCLUDE_DIRS})

if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

if (APPLE)
    set(APPLE_EXTRA_LIBS src/osx_resources.mm)
endif()

add_executable(vector_audio src/main.cpp
                src/application.cpp
                src/config.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui-SFML.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_stdlib.cpp
                ${APPLE_EXTRA_LIBS})

target_link_libraries(vector_audio 
    ${OPENGL_LIBRARY}
    OpenSSL::SSL OpenSSL::Crypto 
    sfml-system sfml-window sfml-graphics
    toml11::toml11
    nlohmann_json nlohmann_json::nlohmann_json
    fmt::fmt-header-only
    afv_native
    ${AFV_LIB} ${CMAKE_THREAD_LIBS_INIT})