cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(vector_audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_path(CPP_HTTPLIB_INCLUDE_DIRS "httplib.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SFML COMPONENTS system window graphics network CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

include_directories(    ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/extern/afv_native/include
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/extern
                        ${CPP_HTTPLIB_INCLUDE_DIRS})

if (APPLE)
    set(AFV_LIB ${CMAKE_SOURCE_DIR}/extern/afv_native/lib/apple/libafv.dylib)
endif()

if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(AFV_LIB ${CMAKE_SOURCE_DIR}/extern/afv_native/lib/libafv.so)
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
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_stdlib.cpp)

target_link_libraries(vector_audio 
    OpenSSL::SSL OpenSSL::Crypto 
    sfml-system sfml-window sfml-graphics
    toml11::toml11
    nlohmann_json nlohmann_json::nlohmann_json
    ${AFV_LIB} ${CMAKE_THREAD_LIBS_INIT})


file(COPY ${AFV_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})