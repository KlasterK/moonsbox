

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
    mainwindowui.hpp mainwindowui.cpp
    fastprng.hpp fastprng.cpp
    ext/miniaudio.h ext/miniaudio.c
    ext/portable-file-dialogs.h
)
target_link_libraries(moonsbox PUBLIC 
    ZLIB::ZLIB PkgConfig::MINIZIP SDL2pp::SDL2pp PkgConfig::SDL2GFX
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")   
    target_compile_options(moonsbox PRIVATE -Wall -Wextra)
endif()
