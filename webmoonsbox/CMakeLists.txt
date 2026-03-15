project(webmoonsbox LANGUAGES C CXX VERSION 1.0.0)

if(NOT EMSCRIPTEN)
    message(WARNING "This project is building without Emscripten. Probably something will go wrong.")
endif()

set(CMAKE_EXECUTABLE_SUFFIX ".js")

add_executable(webmoonsbox webmoonsbox.cpp)
target_link_libraries(webmoonsbox PUBLIC simulationengine)
target_link_options(webmoonsbox PRIVATE
    "SHELL:-s WASM=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s EXPORTED_FUNCTIONS=['_init_game','_tick','_draw_dot']"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']"
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
)
