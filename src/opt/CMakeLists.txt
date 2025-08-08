cmake_minimum_required(VERSION 3.28)
project(moonsbox-opt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set (PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(opt_gamemap gamemap.cpp)
# TODO: make auto moving resulting native module to /src/
