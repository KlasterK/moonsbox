cmake_minimum_required(VERSION 3.20)
project(moonsbox LANGUAGES C CXX VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(IS_TARGET_WEB ON)
    set(IS_TARGET_DESKTOP OFF)
else()
    set(IS_TARGET_WEB OFF)
    set(IS_TARGET_DESKTOP ON)
endif()

option(BUILD_SIMULATIONENGINE "Build simulationengine (simulation part)" ON)
option(BUILD_MOONSBOX "Build moonsbox (application part)" ${IS_TARGET_DESKTOP})
option(BUILD_WEBMOONSBOX "Build moonsbox for WASM (requires Emscripten)" ${IS_TARGET_WEB})

if(BUILD_SIMULATIONENGINE)
    add_subdirectory(simulationengine)
endif()

if(BUILD_MOONSBOX)
    add_subdirectory(moonsbox)
endif()

if(BUILD_WEBMOONSBOX)
    add_subdirectory(webmoonsbox)
endif()
