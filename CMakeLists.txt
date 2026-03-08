cmake_minimum_required(VERSION 3.20)
project(moonsbox LANGUAGES C CXX VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SIMULATIONENGINE "Build simulationengine (simulation part)" ON)
option(BUILD_MOONSBOX "Build moonsbox (application part)" ON)

if(BUILD_SIMULATIONENGINE)
    add_subdirectory(simulationengine)
endif()

if(BUILD_MOONSBOX)
    add_subdirectory(moonsbox)
endif()
