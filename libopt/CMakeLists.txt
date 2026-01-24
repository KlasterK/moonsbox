cmake_minimum_required(VERSION 3.20)
project(moonsbox-opt LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(libopt
    test.cpp
    gamemap.hpp gamemap.cpp
    simulation.hpp simulation.cpp
    materialdefs.hpp
    materialcontroller.hpp
    materials.hpp
    drawing.hpp
)
target_link_libraries(libopt SDL2::SDL2)
