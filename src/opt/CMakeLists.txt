cmake_minimum_required(VERSION 3.20)
project(moonsbox-opt LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python 3.13 COMPONENTS Interpreter Development REQUIRED)
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)

pybind11_add_module(moox-opt
    main.cpp
    gamemap.hpp gamemap.cpp
    renderer.hpp renderer.cpp
    materialdefs.hpp
    materialcontroller.hpp
)
target_include_directories(moox-opt PRIVATE include)
