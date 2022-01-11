cmake_minimum_required(VERSION 3.10)
project(afv_unix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(    ${SDL2_INCLUDE_DIRS}
                        ${OPENGL_INCLUDE_DIRS}
                        ${CMAKE_SOURCE_DIR}/extern/imgui)

add_executable(afv_unix 
                src/main.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_tables.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_draw.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_widgets.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_impl_opengl3.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_impl_sdl.cpp
                ${CMAKE_SOURCE_DIR}/extern/imgui/imgui_demo.cpp)

target_link_libraries(afv_unix ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})