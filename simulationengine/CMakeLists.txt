project(simulationengine LANGUAGES C CXX VERSION 1.0.0)

find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIZIP REQUIRED minizip IMPORTED_TARGET)

add_library(simulationengine
    # Core
    core/materialdefs.hpp
    core/materialcontroller.hpp
    core/gamemap.hpp                        core/gamemap.cpp
    core/materialregistry.hpp               core/materialregistry.cpp
    # Materials
    materials/materials.hpp
    # Algorithms
    algorithms/drawing.hpp                  algorithms/drawing.cpp
    algorithms/fastprng.hpp                 algorithms/fastprng.cpp
    # Simulation
    simulation/simulationmanager.hpp        simulation/simulationmanager.cpp
    # Serialization
    serialization/savecontainer.hpp
    serialization/minizipsavecontainer.hpp  serialization/minizipsavecontainer.cpp
    serialization/saving.hpp                serialization/saving.cpp
)

target_link_libraries(simulationengine PUBLIC ZLIB::ZLIB PkgConfig::MINIZIP)

target_include_directories(simulationengine PUBLIC ..)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(simulationengine PRIVATE -Wall -Wextra)
endif()
