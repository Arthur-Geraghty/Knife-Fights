cmake_minimum_required(VERSION 3.24)

project(KnifeFights LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download raylib automatically
include(FetchContent)

set(RAYLIB_VERSION 6.0)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)

FetchContent_MakeAvailable(raylib)

# Knife Fights executable
add_executable(KnifeFights
    main.cpp
)

target_link_libraries(KnifeFights PRIVATE raylib)

# Windows
if(WIN32)
    target_link_libraries(KnifeFights PRIVATE winmm)
endif()

# Don't open a console window when the game is launched
if(WIN32 AND MSVC)
    target_link_options(KnifeFights PRIVATE
        /SUBSYSTEM:WINDOWS
        /ENTRY:mainCRTStartup
    )
endif()