import locale
import time
from pathlib import Path

import pygame

DEFAULT_TEMP = 300  # 27 *C, 80 *F
TEMP_IS_EXCHANGING = True
DEFAULT_DRAWING_WIDTH = 1
DELTA_DRAWING_WIDTH = 1
ZOOM_FACTOR = 0.1
VOLUME = 0.1
MUSIC_VOLUME = 0.04
DRAWING_IS_CIRCULAR = True
WINDOW_CAPTION = 'moonsbox'

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
            max(0, min(255, (dot.temp - DEFAULT_TEMP) / 50 + 0x7F)),
            grayscale := (dot.color.r + dot.color.g + dot.color.b) / 3,
            grayscale,
        )
    ),
)

PALETTE_ICON_SIZE = 64, 64
PALETTE_MAX_CAPTION_CHARS = 10
PALETTE_MARGIN = 20

MAP_INNER_COLOR = pygame.Color('#00050C')
MAP_OUTER_COLOR = pygame.Color('#000C05')
PALETTE_SELECTION_OUTER_COLOR = pygame.Color("#00C3FF40")
PALETTE_SELECTION_INNER_COLOR = pygame.Color("#008CFF20")
PALETTE_SHADOW_COLOR = pygame.Color('#000000CC')

FONT_SIZE = 15
FONT_NAME_OR_PATH = 'Sans'
FONT_IS_SYSFONT = True

ENABLE_FPS_COUNTER = True
FPS_COUNTER_COLOR = pygame.Color('#00FF00')

USER_LOCALE = locale.getdefaultlocale()[0]
MATERIAL_TRANSLATIONS = {
    'ru_RU': {
        'Space': 'Стереть',
        'Sand': 'Песок',
        'Inert Liquid': 'Жидкость',
        'Unbreakable Wall': 'Неруш. Стена',
        'Lava': 'Лава',
    }
}

ASSETS_ROOT = Path('./assets')

SCREENSHOT_PATH_FACTORY = lambda: time.strftime('./screenshot_%Y-%m-%d_%H-%M-%S.png')
SCREENSHOT_TYPE_HINT = 'PNG'
SCREENSHOT_DOT_SIZE = (1, 1)


def blend(bg: pygame.Color, fg: pygame.Color, alpha: float | None = None) -> pygame.Color:
    '''Blends two colors together.

    If alpha is given, colors' alpha channels will be blended, otherwise
    the foreground color's alpha will be used for blending and alpha channels will not be blended.
    '''

    if alpha is None:
        alpha = fg.a / 0xFF
        blended_alpha_channel = bg.a
    else:
        alpha = max(0, min(1, alpha))
        blended_alpha_channel = int(fg.a * alpha + bg.a * (1 - alpha))

    return pygame.Color(
        int(fg.r * alpha + bg.r * (1 - alpha)),
        int(fg.g * alpha + bg.g * (1 - alpha)),
        int(fg.b * alpha + bg.b * (1 - alpha)),
        blended_alpha_channel,
    )


_images_cache = {}


def get_image(name: str) -> pygame.Surface | None:
    if name in _images_cache:
        return _images_cache[name]

    icon_path = ASSETS_ROOT / name

    try:
        image = pygame.image.load(icon_path).convert_alpha()
    except IOError:
        return None

    _images_cache[name] = image
    return image


def get_material_icon(name: str) -> pygame.Surface | None:
    """Returns a pygame.Surface for the material icon, or None if not found."""

    return get_image('materials/' + name)


_sound_channels = {}
_sounds_cache = {}


class GameSound:
    def __init__(self, name: str):
        channel, _ = name.split('_', 1)

        if channel == 'stream':
            self._channel = None
            pygame.mixer_music.load(ASSETS_ROOT / 'sounds' / name)
            pygame.mixer_music.set_volume(MUSIC_VOLUME)
        else:
            if channel not in _sound_channels:
                channel_id = len(_sound_channels)
                pygame.mixer.set_num_channels(channel_id + 1)
            else:
                channel_id = _sound_channels[channel]
            self._channel = pygame.mixer.Channel(channel_id)

            if name in _sounds_cache:
                self._sound = _sounds_cache[name]
            else:
                try:
                    self._sound = pygame.Sound(ASSETS_ROOT / 'sounds' / name)
                except IOError:
                    self._sound = None
                else:
                    self._sound.set_volume(VOLUME)
                    _sounds_cache[name] = self._sound

    def play(self) -> None:
        if self._channel is None:
            pygame.mixer_music.play(-1)
        else:
            if not self._channel.get_busy() and self._sound is not None:
                self._channel.play(self._sound)

    def play_override(self) -> None:
        if self._channel is None:
            pygame.mixer_music.stop()
            pygame.mixer_music.play(-1)
        elif self._sound is not None:
            self._channel.stop()
            self._channel.play(self._sound)


def get_font() -> pygame.font.Font:
    """Returns a pygame.font.Font object for the given size."""

    if FONT_IS_SYSFONT:
        return pygame.font.SysFont(FONT_NAME_OR_PATH, FONT_SIZE)
    else:
        return pygame.font.Font(FONT_NAME_OR_PATH, FONT_SIZE)
