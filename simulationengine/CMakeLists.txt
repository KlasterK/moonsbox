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
    # Algorithms
    algorithms/drawing.hpp                  algorithms/drawing.cpp
    algorithms/fastprng.hpp                 algorithms/fastprng.cpp
    # Simulation
    simulation/simulationmanager.hpp        simulation/simulationmanager.cpp
    # Serialization
    serialization/savecontainer.hpp
    serialization/minizipsavecontainer.hpp  serialization/minizipsavecontainer.cpp
    serialization/saving.hpp                serialization/saving.cpp
    # Materials
    materials/common.hpp
    materials/allmaterials.hpp
    materials/absorbent.hpp
    materials/aerogel.hpp
    materials/blackhole.hpp
    materials/dryice.hpp
    materials/fire.hpp
    materials/ice.hpp
    materials/lava.hpp
    materials/minus100k.hpp
    materials/plus100k.hpp
    materials/propane.hpp
    materials/pureglass.hpp
    materials/sand.hpp
    materials/space.hpp
    materials/steam.hpp
    materials/tap.hpp
    materials/unbreakablewall.hpp
    materials/water.hpp
)

target_link_libraries(simulationengine PUBLIC ZLIB::ZLIB PkgConfig::MINIZIP)

target_include_directories(simulationengine PUBLIC ..)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(simulationengine PRIVATE -Wall -Wextra)
endif()
