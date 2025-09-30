set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

# .pyd/.so files will be in /src
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
# helpers for MSVC
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/..")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/..")

pybind11_add_module(game pythonapi.cpp)
target_link_libraries(game PRIVATE core)
