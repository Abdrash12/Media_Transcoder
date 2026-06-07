cmake_minimum_required(VERSION 3.20)
project(HighPerfTranscoder VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Libraries 
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswresample)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# Find system threads
find_package(Threads REQUIRED)

#  Include ImGui 
include_directories(external/imgui external/imgui/backends)
include_directories(C:/msys64/ucrt64/include)

# Core Application Target
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/Core/ConversionManager.cpp
    src/Core/MP4ToMP3Converter.cpp
    src/GUI/MainWindow.cpp
    # ImGui Core
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    # ImGui Backends
    external/imgui/backends/imgui_impl_sdl2.cpp
    external/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    PkgConfig::FFMPEG 
    PkgConfig::SDL2 
    Threads::Threads
)