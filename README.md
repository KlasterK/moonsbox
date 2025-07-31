# moonsbox

A sandbox cellular automata playground written in Python with Pygame. Inspired by classic falling sand games, especially SmellyMoo's sand:box, moonsbox lets you experiment with different materials and their interactions in a pixel world.

## Features

- **Material palette**: Choose materials with a visual palette (keyboard/mouse navigation supported)
- **Heat simulation**: Materials exchange temperature based on heat capacity and thermal conductivity
- **Sound effects**: Each material can play sounds on interaction
- **Resizable window**: The simulation area and camera can be zoomed and panned
- **Screenshot**: Save or view the current simulation as an image
- **Highly customizable**: Tune your sandbox changing constants or assets

## Controls

- **LMB | Ctrl**: Draw with the selected material
- **RMB | Shift**: Move the camera
- **Mouse Wheel**: Zoom camera
- **MMB | Tab**: Open the material palette
- **Arrow Keys | Mouse**: Move material selection
- **Enter | Double LMB**: Select material to draw
- **Esc**: Close palette or exit
- **Space**: Pause simulation
- **F1**: Change mask
- **F5**: Step simulation
- **F6**: Clean map
- **F9**: Show map screenshot
- **F10**: Save map screenshot
- **+ | -**: Change brush size
- **R**: Pause rendering to speed up simulation

## Materials

Some example materials you can experiment with:

- **Sand**: Falls and piles up, turns into glass if heated.
- **Lava**: Hot, flows and heats up surroundings. Gets solid if colded.
- **±100 K**: Increases/decreases temperature of dot.

> The full list of available materials is shown in the material palette in the app.

## Requirements

- Python 3 (I run it with Python 3.13.3)
- pygame-ce
- numpy
- pillow

## Running

```
python -m pip install pygame-ce numpy pillow
cd <PROJECT DIR>
python -m src
```