cmake_minimum_required(VERSION 3.20)
project(moonsbox-opt LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(SDL2pp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIZIP minizip REQUIRED)

add_executable(moonsbox
    main.cpp
    gamemap.hpp gamemap.cpp
    simulation.hpp simulation.cpp
    materialdefs.hpp
    materialcontroller.hpp
    materials.hpp
    drawing.hpp drawing.cpp
    renderer.hpp renderer.cpp
    materialregistry.hpp materialregistry.cpp
    savecontainer.hpp
    minizipsavecontainer.hpp minizipsavecontainer.cpp
    saving.hpp saving.cpp
    camera.hpp camera.cpp
    materialpalette.hpp materialpalette.cpp
    windowevents.hpp windowevents.cpp
    gameapp.hpp gameapp.cpp
    soundsystem.hpp soundsystem.cpp
    fpscounter.hpp fpscounter.cpp
    ext/miniaudio.h ext/miniaudio.c
)
target_link_libraries(moonsbox PUBLIC ZLIB::ZLIB minizip SDL2pp::SDL2pp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(moonsbox PRIVATE -Wall -Wextra)
endif()
