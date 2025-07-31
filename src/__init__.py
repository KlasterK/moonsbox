import dataclasses
import locale
import random
import time
from pathlib import Path

import pygame

DEFAULT_TEMP = 300  # 27 *C, 80 *F
TEMP_IS_EXCHANGING = True
DEFAULT_DRAWING_WIDTH = 1
DELTA_DRAWING_WIDTH = 1
ZOOM_FACTOR = 0.1
VOLUME = 1
MUSIC_VOLUME = 0
DRAWING_IS_CIRCULAR = True
WINDOW_CAPTION = 'moonsbox'

SCREEN_SIZE = 400, 400
MAP_SIZE = 100, 100
VISIBLE_AREA = pygame.Rect(-20, -20, 140, 140)

PALETTE_ICON_SIZE = 64, 64
PALETTE_MAX_CAPTION_CHARS = 20
PALETTE_MARGIN = 20

MAP_INNER_COLOR = pygame.Color('#000000')
MAP_OUTER_COLOR = pygame.Color("#001A0B")
PALETTE_SELECTION_OUTER_COLOR = pygame.Color("#00C3FF40")
PALETTE_SELECTION_INNER_COLOR = pygame.Color("#008CFF20")
PALETTE_SHADOW_COLOR = pygame.Color('#000000CC')

FONT_SIZE = 15
FONT_NAME_OR_PATH = 'Sans'
FONT_IS_SYSFONT = True

ENABLE_VSYNC = True
ENABLE_FPS_COUNTER = False
ENABLE_TPS_COUNTER = False
DEBUG_COLOR = pygame.Color('#00FF00')

USER_LOCALE = locale.getdefaultlocale()[0]
MATERIAL_TRANSLATIONS = {
    'ru_RU': {
        'Space': 'Стереть',
        'Sand': 'Песок',
        'Lubricant': 'Смаз. Масло',
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


def get_font() -> pygame.font.Font:
    """Returns a pygame.font.Font object for the given size."""

    if FONT_IS_SYSFONT:
        return pygame.font.SysFont(FONT_NAME_OR_PATH, FONT_SIZE)
    else:
        return pygame.font.Font(FONT_NAME_OR_PATH, FONT_SIZE)


# from profilehooks import profile
# @profile(stdout=False, filename='GameSound.prof')
class GameSound:
    _channels: dict[str, int] = {}
    _sound_cache: dict[str, list[pygame.mixer.Sound]] = {}
    _sequential_indices: dict[str, int] = {}
    _failed_sound_paths: set[Path] = set()
    _sequential_cache: dict[str, Path] = {}

    # from profilehooks import profile
    # @profile(stdout=False, filename='GameSound-__init__.prof')
    def __init__(self, pattern: str):
        # category ('+' param)* '.' sound_name ('.' extension)?
        prefix, _, _ = pattern.partition('.')
        category, *params = prefix.split('+')

        self._is_stream = category == 'stream'
        self._sound = None
        self._is_sequential = 'sequential' in params

        if pattern in GameSound._sequential_cache:
            self._sound_variants = GameSound._sequential_cache[pattern]
        else:
            sound_dir = ASSETS_ROOT / 'sounds'
            variants = sound_dir.glob(pattern + '.*')
            if not variants:
                GameSound._sequential_cache[pattern] = []
                return
            GameSound._sequential_cache[pattern] = self._sound_variants = (
                sorted(variants) if self._is_sequential else list(variants)
            )

        if self._is_sequential:
            if pattern not in self._sequential_indices:
                self._current_index = self._sequential_indices[pattern] = 0
            else:
                self._current_index = self._sequential_indices[pattern]
        else:
            self._current_index = None

        if self._sound_variants:
            self._sound_path = (
                self._sound_variants[self._current_index % len(self._sound_variants)]
                if self._is_sequential
                else random.choice(self._sound_variants)
            )

        if self._is_stream:
            self._channel = None
            self._load_stream()
        else:
            self._init_channel(category)
            self._load_sound()

    def _init_channel(self, category) -> None:
        if category not in GameSound._channels:
            channel_id = len(GameSound._channels)
            pygame.mixer.set_num_channels(channel_id + 1)
            GameSound._channels[category] = channel_id
            self._channel = pygame.mixer.Channel(channel_id)
        else:
            self._channel = pygame.mixer.Channel(GameSound._channels[category])

    def _load_stream(self) -> None:
        if not self._sound_variants or self._sound_path in self._failed_sound_paths:
            return

        try:
            pygame.mixer.music.load(str(self._sound_path))
            pygame.mixer.music.set_volume(MUSIC_VOLUME)
        except pygame.error:
            self._failed_sound_paths.add(self._sound_path)

    def _load_sound(self) -> None:
        if not self._sound_variants or self._sound_path in self._failed_sound_paths:
            return

        if self._sound_path in GameSound._sound_cache:
            self._sound = GameSound._sound_cache[self._sound_path]
        else:
            try:
                self._sound = pygame.mixer.Sound(str(self._sound_path))
                self._sound.set_volume(VOLUME)
                GameSound._sound_cache[self._sound_path] = self._sound
            except pygame.error:
                self._failed_sound_paths.add(self._sound_path)
                self._sound = GameSound._sound_cache[self._sound_path] = None

    def play(self):
        if self._is_stream:
            pygame.mixer.music.play(-1)
            return

        if self._sound is None or not self._channel or self._channel.get_busy():
            return

        self._channel.play(self._sound)

        if self._is_sequential:
            self._current_index += 1
            self._sequential_indices[self._sound_path] = self._current_index % len(
                self._sound_variants
            )

    def play_override(self) -> None:
        if self._is_stream:
            pygame.mixer.music.stop()
            pygame.mixer.music.play(-1)
            return

        if self._sound is None or not self._channel:
            return

        self._channel.stop()
        self._channel.play(self._sound)

        if self._is_sequential:
            self._current_index += 1
            self._sequential_indices[self._sound_path] = self._current_index % len(
                self._sound_variants
            )
