cmake_minimum_required(VERSION 3.28)
project(moonsbox-opt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
add_subdirectory(core)
add_subdirectory(modapi)
add_subdirectory(pythonapi)
