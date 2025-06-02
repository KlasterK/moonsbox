import time
from pathlib import Path
from typing import Any

import pygame

DEFAULT_TEMP = 300  # 27 *C, 80 *F
DEFAULT_WIDTH = 1
DELTA_WIDTH = 1
ZOOM_FACTOR = 0.1
VOLUME = 0.1
MUSIC_VOLUME = 0.05

SCREEN_SIZE = 300, 300
MAP_SIZE = 100, 100
VISIBLE_AREA = pygame.Rect(-20, -20, 140, 140)
RENDER_MASKS = (
    # Normal color mask
    (lambda dot: dot.color),
    # R = temperature, G = B = grayscale
    (
        lambda dot: pygame.Color(
            max(0, min(255, dot.temp - DEFAULT_TEMP + 0x7F)),
            grayscale := (dot.color.r + dot.color.g + dot.color.b) / 3,
            grayscale,
        )
    ),
    # R = temperature (wider range), G = B = grayscale
    (
        lambda dot: pygame.Color(
            max(0, min(255, (dot.temp - DEFAULT_TEMP) / 20 + 0x7F)),
            grayscale := (dot.color.r + dot.color.g + dot.color.b) / 3,
            grayscale,
        )
    ),
)

PALETTE_ICON_SIZE = 64, 64
PALETTE_MAX_CAPTION_CHARS = 10
PALETTE_MARGIN = 20

MAP_INNER_COLOR = pygame.Color('#000000')
MAP_OUTER_COLOR = pygame.Color('#000C05')
PALETTE_SELECTION_OUTER_COLOR = pygame.Color("#00C3FF40")
PALETTE_SELECTION_INNER_COLOR = pygame.Color("#008CFF20")
PALETTE_SHADOW_COLOR = pygame.Color('#000000CC')

FONT_SIZE = 15
FONT_NAME_OR_PATH = 'Century Schoolbook'
FONT_IS_SYSFONT = True

ENABLE_FPS_LINE = True
FPS_LINE_COLOR = pygame.Color('#00FF00')

MATERIAL_TRANSLATIONS = {
    'Space': 'Стереть',
    'Sand': 'Песок',
    'Inert Liquid': 'Жидкость',
    'Unbreakable Wall': 'Неруш. Стена',
    'Lava': 'Лава',
}

ASSETS_ROOT = Path('./assets')

SCREENSHOT_PATH_FACTORY = lambda: time.strftime('./screenshot_%Y-%m-%d_%H-%M-%S.png')
SCREENSHOT_TYPE_HINT = 'PNG'


def blend(bg: pygame.Color, fg: pygame.Color) -> pygame.Color:
    fg_alpha = fg.a / 255.0

    return pygame.Color(
        int(fg.r * fg_alpha + bg.r * (1 - fg_alpha)),
        int(fg.g * fg_alpha + bg.g * (1 - fg_alpha)),
        int(fg.b * fg_alpha + bg.b * (1 - fg_alpha)),
        255,
    )


_material_icons_cache = {}


def get_material_icon(name: str) -> pygame.Surface | None:
    """Returns a pygame.Surface for the material icon, or None if not found."""

    if name in _material_icons_cache:
        return _material_icons_cache[name]

    icon_path = ASSETS_ROOT / 'materials' / name

    try:
        icon = pygame.image.load(icon_path).convert_alpha()
    except IOError:
        return None

    _material_icons_cache[name] = icon
    return icon


_sounds_cache = {}


class GameSound:
    def __init__(self, name: str):
        self._name = name

        if name in _sounds_cache:
            game_sound = _sounds_cache[name]
            self._sound, self._last_play_timestamp = game_sound

        else:
            sound_path = ASSETS_ROOT / 'sounds' / name

            try:
                self._sound = pygame.mixer.Sound(sound_path)
            except IOError:
                self._sound = None

            self._sound.set_volume(VOLUME)
            _sounds_cache[name] = self._sound, 0

            self._last_play_timestamp = 0

    def play(self) -> None:
        if self._sound is None:
            return

        timestamp = time.time()
        required_timestamp = self._last_play_timestamp + self._sound.get_length()
        if timestamp < required_timestamp:
            return

        self._last_play_timestamp = timestamp
        _sounds_cache[self._name] = _sounds_cache[self._name][0], self._last_play_timestamp
        self._sound.play()

    def play_override(self) -> None:
        if self._sound is None:
            return

        self._last_play_timestamp = time.time()
        _sounds_cache[self._name] = _sounds_cache[self._name][0], self._last_play_timestamp

        self._sound.stop()
        self._sound.play()


def get_font() -> pygame.font.Font:
    """Returns a pygame.font.Font object for the given size."""

    if FONT_IS_SYSFONT:
        return pygame.font.SysFont(FONT_NAME_OR_PATH, FONT_SIZE)
    else:
        return pygame.font.Font(FONT_NAME_OR_PATH, FONT_SIZE)
