project(webmoonsbox LANGUAGES C CXX VERSION 1.0.0)

if(NOT EMSCRIPTEN)
    message(WARNING "This project is building without Emscripten. Probably something will go wrong.")
endif()

set(CMAKE_EXECUTABLE_SUFFIX ".js")

set(EXPORTED_FUNCTIONS
    init_game
    tick
    set_dot_at
    draw_rect_or_ellipse_non_destructive
    draw_line_non_destructive
    play_place_sound
    register_play_sound_callback
    minizip_load_store
)
string(JOIN "','_" EXPORTED_FUNCTIONS_JOINED_MID ${EXPORTED_FUNCTIONS})

include(FetchContent)
FetchContent_Declare(
    minizip
    GIT_REPOSITORY https://github.com/domoticz/minizip.git
)
FetchContent_MakeAvailable(minizip)

target_link_libraries(simulationengine PUBLIC minizip)
target_include_directories(simulationengine PUBLIC ${minizip_SOURCE_DIR})

add_executable(webmoonsbox webmoonsbox.cpp)
target_link_libraries(webmoonsbox PUBLIC simulationengine)
target_link_options(webmoonsbox PRIVATE
    "SHELL:-s WASM=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s EXPORTED_FUNCTIONS=['_${EXPORTED_FUNCTIONS_JOINED_MID}']"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','addFunction']"
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:-s RESERVED_FUNCTION_POINTERS=10"
    "SHELL:-s ALLOW_TABLE_GROWTH=1"
    "SHELL:-s USE_ZLIB=1"
)
