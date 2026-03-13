project(simulationengine LANGUAGES C CXX VERSION 1.0.0)

option(SIMULATIONENGINE_BUILD_MINIZIPSAVECONTAINER
    "Find minizip and build minizipsavecontainer.cpp"
    ON
)

add_library(simulationengine
    # Core
    core/materialdefs.hpp
    core/materialcontroller.hpp
    core/gamemap.hpp                        core/gamemap.cpp
    core/materialregistry.hpp               core/materialregistry.cpp
    # Algorithms
    algorithms/drawing.hpp                  algorithms/drawing.cpp
    algorithms/fastprng.hpp                 algorithms/fastprng.cpp
    algorithms/cksum.hpp                    algorithms/cksum.cpp
    # Simulation
    simulation/simulationmanager.hpp        simulation/simulationmanager.cpp
    # Serialization
    serialization/savecontainer.hpp
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
target_include_directories(simulationengine PUBLIC ..)

if(SIMULATIONENGINE_BUILD_MINIZIPSAVECONTAINER)
    find_path(MINIZIP_INCLUDE_DIR
        NAMES minizip/zip.h minizip/unzip.h
        REQUIRED
        DOC "Path to <minizip/zip.h> and <minizip/unzip.h>"
    )
    find_library(MINIZIP_LIBRARY
        NAMES minizip libminizip
        REQUIRED
        DOC "Path to minizip library"
    )

    target_sources(simulationengine PRIVATE
        serialization/minizipsavecontainer.hpp  serialization/minizipsavecontainer.cpp
    )
    target_include_directories(simulationengine PUBLIC ${MINIZIP_INCLUDE_DIR})
    target_link_libraries(simulationengine PUBLIC ${MINIZIP_LIBRARY})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(simulationengine PRIVATE -Wall -Wextra)
endif()
