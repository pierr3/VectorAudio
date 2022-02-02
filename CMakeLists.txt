cmake_minimum_required(VERSION 3.10)
project(afv_unix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

#include(${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake)
#conan_basic_setup(TARGETS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(OpenGL REQUIRED)
find_package(SFML 2.5 COMPONENTS system window graphics)

include_directories(    ${SDL2_INCLUDE_DIRS}
                        ${OPENGL_INCLUDE_DIRS}
                        ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/extern/afv_native/include
                        ${CMAKE_SOURCE_DIR}/extern/imgui
                        ${CMAKE_SOURCE_DIR}/extern)

add_executable(afv_unix src/main.cpp
                src/application.cpp
                src/config.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui-SFML.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_stdlib.cpp)

if (APPLE)
    set(AFV_LIB ${CMAKE_SOURCE_DIR}/extern/afv_native/lib/arm64/libafv.dylib)
endif()

target_link_libraries(afv_unix ${OPENGL_LIBRARIES} sfml-system sfml-window sfml-graphics ${AFV_LIB})

install(TARGETS afv_unix
    RUNTIME DESTINATION ${INSTALL_DIR})

file(COPY ${AFV_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})