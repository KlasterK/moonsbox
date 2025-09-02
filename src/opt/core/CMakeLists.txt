add_library(core)
target_sources(core
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
    util.cppm
    materialregistry.cppm
    gamemap.cppm
    modloader.cppm
    simulation.cppm
    renderer.cppm
)
