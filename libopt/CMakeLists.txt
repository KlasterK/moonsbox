cmake_minimum_required(VERSION 3.20)
project(moonsbox-opt LANGUAGES C CXX VERSION 0.1.0)

set(LIBOPT_INSTALL_DIR 
    "${CMAKE_SOURCE_DIR}/../src" 
    CACHE PATH 
    "Directory to install pyd/so file"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

find_package(Python 3.13 COMPONENTS Interpreter Development REQUIRED)
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)

pybind11_add_module(libopt
    module.cpp
    gamemap.hpp gamemap.cpp
    simulation.hpp simulation.cpp
    materialdefs.hpp
    materialcontroller.hpp
    materials.hpp
    drawing.hpp
    renderer.hpp renderer.cpp
    materialregistry.hpp materialregistry.cpp
    savecontainer.hpp
    minizipsavecontainer.hpp minizipsavecontainer.cpp

)
target_link_libraries(libopt PUBLIC SDL2::SDL2)
target_precompile_headers(libopt PRIVATE
    <SDL2/SDL.h>
    <pybind11/pybind11.h>
    <pybind11/stl.h>
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(libopt PRIVATE -Wall -Wextra)
endif()

# pygame-ce
target_include_directories(libopt PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_definitions(libopt PRIVATE
    PG_MAJOR_VERSION=2
    PG_MINOR_VERSION=5  
    PG_PATCH_VERSION=7
    PG_VERSION_TAG=".dev1"
)

get_filename_component(ABS_DST ${LIBOPT_INSTALL_DIR} ABSOLUTE)
install(TARGETS libopt
    LIBRARY DESTINATION ${ABS_DST}
)
