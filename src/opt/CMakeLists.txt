cmake_minimum_required(VERSION 3.28)
project(moonsbox-opt LANGUAGES C CXX)

# .pyd/.so files will be in /src
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
# helpers for MSVC
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/..")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/..")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(opt)
target_sources(opt 
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        gamemap.cppm
        simulation.cppm
        renderer.cppm
        materials.cppm
        public.cpp
)

