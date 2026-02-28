cmake_minimum_required(VERSION 3.20)
project(moonsbox LANGUAGES C CXX VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(SDL2pp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIZIP REQUIRED minizip IMPORTED_TARGET)
pkg_check_modules(SDL2GFX REQUIRED SDL2_gfx IMPORTED_TARGET)

add_subdirectory(src)
