# moonsbox

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
- **Highly customizable**: Tune your sandbox changing constants or assets

## Plans
- Move consts to a config file.
- Get rid of tkinter.filedialog dependency and make our own opener of native file dialog.
- Make the first release of the game.
- Add alternative to keyboard controls in form of widgets.
- Add full touchscreen support.
- Make a release for Android.
- Add visual config editing and all widgets are needed for it.
- Start developing native (C++) version of gamemap.py.
- Native version of simulation.py.
- Native version of renderer.py.
- Native version of materials.py (but keep support of Python-written classes).
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

## Requirements

- Python 3 (I run it with Python 3.13.3)
- tkinter (tk in PyPI)
- pygame-ce
- numpy
- pillow

## Running

```
python -m pip install tk pygame-ce numpy pillow
cd <PROJECT DIR>
python -m src
```