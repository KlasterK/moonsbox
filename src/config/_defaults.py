from dataclasses import dataclass, field
from pathlib import Path
from typing import Literal
import pygame
import locale
import time


@dataclass
class ConfigDefaults:
    DEFAULT_TEMP: int = 300
    TEMP_IS_EXCHANGING: bool = True
    DEFAULT_DRAWING_WIDTH: int = 1
    DELTA_DRAWING_WIDTH: int = 1
    ZOOM_FACTOR: float = 0.1
    VOLUME: float = 1
    MUSIC_VOLUME: float = 0
    DRAWING_IS_CIRCULAR: bool = True
    DRAWING_IS_DESTRUCTIVE: bool = False
    WINDOW_CAPTION: str = "moonsbox"
    SCREEN_SIZE: tuple = (560, 560)
    MAP_SIZE: tuple = (100, 100)
    VISIBLE_AREA: pygame.Rect = field(default_factory=lambda: pygame.Rect(-20, -20, 140, 140))
    PALETTE_ICON_SIZE: tuple = (64, 64)
    PALETTE_MAX_CAPTION_CHARS: int = 20
    PALETTE_MARGIN: int = 20
    MAP_INNER_COLOR: pygame.Color = field(default_factory=lambda: pygame.Color("#000000"))
    MAP_OUTER_COLOR: pygame.Color = field(default_factory=lambda: pygame.Color("#071b10"))
    PALETTE_SELECTION_OUTER_COLOR: pygame.Color = field(
        default_factory=lambda: pygame.Color("#00C3FF40")
    )
    PALETTE_SELECTION_INNER_COLOR: pygame.Color = field(
        default_factory=lambda: pygame.Color("#008CFF20")
    )
    PALETTE_SHADOW_COLOR: pygame.Color = field(default_factory=lambda: pygame.Color("#000000CC"))
    FONT_SIZE: int = 15
    FONT_IDENTIFIER: str = ''
    FONT_SOURCE: Literal['file', 'system', 'auto'] = 'auto'
    ENABLE_VSYNC: bool = True
    ENABLE_FPS_COUNTER: bool = False
    ENABLE_TPS_COUNTER: bool = False
    DEBUG_COLOR: pygame.Color = field(default_factory=lambda: pygame.Color("#00FF00"))
    USER_LOCALE: str = field(default_factory=lambda: locale.getlocale()[0])
    MATERIAL_TRANSLATIONS: dict = field(
        default_factory=lambda: {
            "ru_RU": {
                "Space": "Стереть",
                "Sand": "Песок",
                "Water": "Вода",
                'Ice': 'Лёд',
                'Steam': 'Пар',
                "Unbreakable Wall": "Неруш. Стена",
                "Lava": "Лава",
                "+100 K": "+100 °C",
                "-100 K": "-100 °C",
                "Black Hole": "Чёрная Дыра",
                "Tap": "Кран",
                "Propane": "Пропан",
                "Fire": "Огонь",
                "Glass": "Стекло",
                'Absorbent': 'Абсорбент',
                'Aerogel': 'Аэрогель',
                'Dry Ice': 'Сухой Лёд',
            },
            "en_US": {
                "+100 K": "+180 °F",
                "-100 K": "-180 °F",
            },
            'zh_CN': {
                "Space": "清除",
                "Sand": "沙子",
                "Water": "水",
                'Ice': "冰",
                'Steam': "蒸汽",
                'Unbreakable Wall': "强化墙",
                'Lava': "熔岩",
                '+100 K': "加温100 °C",
                '-100 K': "冷却100 °C",
                'Black Hole': "黑洞",
                'Tap': "水龙头",
                'Propane': "丙烷",
                'Fire': "火",
                'Glass': "玻璃",
                'Absorbent': "吸收剂",
                'Aerogel': "气凝胶",
                'Dry Ice': "干冰",
            },
        }
    )
    ASSETS_ROOT: Path = Path("./assets")
    SCREENSHOT_PATH_FACTORY: callable = lambda: time.strftime("./screenshot_%Y-%m-%d_%H-%M-%S.png")
    SCREENSHOT_TYPE_HINT: str = "PNG"
    SCREENSHOT_DOT_SIZE: tuple = (1, 1)
    CAPTURE_PATH_FACTORY: callable = lambda: time.strftime("./capture_%Y-%m-%d_%H-%M-%S/")
    PALETTE_MOUSE_SELECTION_REQUIRES_DBL_CLICK: bool = True
    MAX_TPS: float = 0
    MAX_FPS: float = 0
