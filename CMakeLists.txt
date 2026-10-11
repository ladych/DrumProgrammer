cmake_minimum_required(VERSION 3.22)

project(DrumProgrammer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows: static MSVC runtime, so DrumProgrammer.exe runs on machines without the
# Visual C++ Redistributable (testers get only the exe). Applies to JUCE and GoogleTest too.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(DRUMPROG_BUILD_APP "Build the JUCE desktop application" ON)
option(DRUMPROG_BUILD_TESTS "Build the unit tests" ON)
option(DRUMPROG_COVERAGE "Instrument the build for coverage (GCC/Clang)" OFF)
option(DRUMPROG_ASIO "Windows: build with ASIO (Steinberg ASIO SDK, GPLv3)" ON)
set(DRUMPROG_ASIO_SDK_DIR "" CACHE PATH "Windows: local ASIO SDK directory (contains common/iasiodrv.h); empty downloads it")
set(DRUMPROG_SANITIZERS "" CACHE STRING "Comma-separated sanitizers, e.g. address,undefined or thread")

include(cmake/DrumProgrammerOptions.cmake)

include(cmake/DrumProgrammerJuce.cmake)

add_subdirectory(src)

if(DRUMPROG_BUILD_TESTS)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/googletest EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()
