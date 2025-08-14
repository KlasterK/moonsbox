# moonsbox

![Gameplay Demo](doc-resources/moonsbox-map-preview.gif)

A sandbox cellular automata playground written in Python with Pygame. Inspired by classic falling 
sand games, especially SmellyMoo's sand:box, moonsbox lets you experiment with different materials
and their interactions in a pixel world.

## Features

- **Material palette**: Choose materials with a visual palette (keyboard/mouse navigation supported)
- **Heat simulation**: Materials exchange temperature based on heat capacity and 
                       thermal conductivity
- **Sound effects**: Some materials can play sounds on interaction
- **Resizable window**: The simulation area and camera can be zoomed and panned
- **Saving & Screenshot**: Share some funny situations with your friends using saves or screenshots
- **Highly customizable**: Tune your sandbox changing TOML config or assets

## Plans

- Move const.py to a config file. (✓)
- Get rid of tkinter.filedialog dependency and make our own opener of native file dialog. (✓)
- Make capturing system for the game on top of FFMPEG. (✓)
- Make the first release of the game. (✓)
- Spread the game.
- Create a chat for the game.
- Start developing native (C++) version of gamemap.py.
- Native version of simulation.py.
- Native version of renderer.py.
- Native version of materials.py (but keep support of Python-written classes).
- Add more materials.
- Add alternative to keyboard controls in form of widgets.
- Add full touchscreen support.
- Make a release for Android.
- Add visual config editing and all widgets are needed for it.
- Add more and more materials.

## Controls

- **LMB | Ctrl**: Draw with the selected material
- **RMB | Shift**: Move the camera
- **Mouse Wheel**: Zoom camera
- **MMB | Tab**: Open the material palette
- **Arrow Keys | Mouse**: Move material selection
- **Enter | Double LMB**: Select material to draw
- **Esc**: Close palette or exit
- **Space**: Pause simulation
- **F1**: Change render mask (normal/thermal)
- **F5**: Step simulation
- **F6**: Clean map
- **F9**: Show map screenshot
- **F10**: Save map screenshot
- **+ | -**: Change brush size
- **R**: Pause rendering to speed up simulation
- **Alt + F12**: Begin or end capturing (see Running to convert capture to a video)

## Requirements

- Python 3 (I run it with Python 3.13.3)
- pygame-ce
- numpy
- pillow
- toml

## Running

```
# If your platform is not Windows, it's recommended to have tkinter:
python -c "import tkinter" # checks if tkinter is here
# If you don't have one and not using Windows, then if you open file dialogs, you have to enter
# text into console.

# Getting ready
python -m pip install pygame-ce numpy pillow toml
cd <PROJECT DIR>

# Settings are at ./user/config.toml
notepad user\config.toml # if you have Windows
nano user/config.toml # if you have GNU nano

# Running
python -m src

# Converting a capture to a video
# Requires FFMPEG!
ffmpeg -i <CAPTURE DIR>/frame_%06d.png [<further args>]
# Create a video with 60 FPS, x264 codec, scaled to 400x400 (but still pixelated):
ffmpeg -i ./capture_2025-08-05_17-02-33/frame_%06d.png -framerate 60 -vcodec libx264 \
       -sws_flags neighbor -s 400x400 ./testcapt.mp4
rm -rf ./capture_2025-08-05_17-02-33/
```

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.
