project(moonsbox LANGUAGES C CXX VERSION 2.0.0)

find_package(SDL2 REQUIRED)
find_package(SDL2pp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2GFX REQUIRED SDL2_gfx IMPORTED_TARGET)

add_executable(moonsbox
    main.cpp
    camera.hpp camera.cpp
    materialpalette.hpp materialpalette.cpp
    windowevents.hpp windowevents.cpp
    gameapp.hpp gameapp.cpp
    soundsystem.hpp soundsystem.cpp
    fpscounter.hpp fpscounter.cpp
    mainwindowui.hpp mainwindowui.cpp
    renderer.hpp renderer.cpp
    ext/miniaudio.h ext/miniaudio.c
    ext/portable-file-dialogs.h
)

target_link_libraries(moonsbox PUBLIC 
    SDL2::SDL2 SDL2pp::SDL2pp PkgConfig::SDL2GFX simulationengine
)

target_include_directories(moonsbox PRIVATE ..)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(moonsbox PRIVATE -Wall -Wextra)
endif()
