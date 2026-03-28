# moonsbox

![Gameplay Demo](doc-resources/moonsbox-map-preview.gif)

A falling sand game written in C++ with SDL2. 
Inspired by established falling sand games, 
moonsbox lets you experiment with different materials
and their interactions in a 2D grid world with great performance.

The main inspiration for moonsbox is [sand:box](https://smellymoo.com/) by smellymoo, along with 
[Sandboxels](https://sandboxels.r74n.com/), 
[Powder Game](https://play.google.com/store/apps/details?id=jp.danball.powdergameviewer) and 
[The Powder Toy](https://powdertoy.co.uk/).

## Web Version

[Play web version!](https://klasterk.github.io/moonsbox/)
No need to download any executables, just **click and play**.

## Features

- **Material palette**: Choose materials with a visual palette
- **Heat simulation**: Temperature exchange based on heat capacity and 
                       thermal conductivity of materials
- **Sounds of gameplay**: Some materials can play sounds on interaction
- **Advanced renderer**: Zoom and heatmap display are supported
- **Saving system**: Save your progress, share your buildings, schemes or silly states
- **Standalone simulation**: Simulation can be run headless.
                             You can code a frontend for any platform

## Social

Join our community!

- For English speakers, come chat with us on [discord server](https://discord.gg/ZKDg6KHqyQ).
- Если вы знаете русский, подписывайтесь на наш [телеграм канал](https://t.me/kk_moonsbox).
  Новости там появляются быстрее, чем в дискорде.
  (Telegram channel. News appear there faster.)

## Building from sources

### Dependencies

Common:

- C++23
- CMake 3.20+

simulationengine (simulation backend):

- minizip (may be disabled)
- pybind11 (optional, for Python API)

moonsbox (desktop SDL2 frontend):

- SDL2 (including SDL_ttf, SDL_image)
- SDL2_gfx
- SDL2pp

webmoonsbox (web frontend):

- Emscripten SDK
- zlib

Installing dependencies in Arch-based distros:

```sh
# Update your system
sudo pacman -Syu
# simulationengine
sudo pacman -S base-devel cmake minizip
# moonsbox
sudo pacman -S sdl3 sdl2-compat sdl2_gfx sdl2_image sdl2_ttf
yay -S sdl2pp
# webmoonsbox
yay -S emsdk
emsdk install latest
embuilder build zlib
```

### Building and running commands

```sh
# Desktop version
git clone https://github.com/KlasterK/moonsbox.git
cd moonsbox
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
build/moonsbox/moonsbox

# Web version
emsdk activate latest
. emsdk_env.sh
git clone https://github.com/KlasterK/moonsbox.git
cd moonsbox
emcmake cmake -S. -Bbuild-wasm 
cmake --build build-wasm -j$(nproc)
python -m http.server &
xdg-open http://127.0.0.1:8000/webmoonsbox/index.html
```

## License

This project is licensed under the GPL-3.0 License -- see the [LICENSE](LICENSE) file for details.
