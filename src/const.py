import locale
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
DRAWING_IS_DESTRUCTIVE = False
WINDOW_CAPTION = 'moonsbox'

SCREEN_SIZE = 560, 560
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
