from dataclasses import dataclass
from typing import Literal

import pygame

from .config import (
    ASSETS_ROOT,
    FONT_IDENTIFIER,
    FONT_SOURCE,
    FONT_SIZE,
    USER_LOCALE,
)


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


def get_image(
    name: str,
    scale_to: tuple[int, int] | None = None,
    scale_type: Literal['smooth', 'pixel'] = 'pixel',
) -> pygame.Surface | None:
    if name in _images_cache:
        image = _images_cache[name]
    else:
        icon_path = ASSETS_ROOT / name
        try:
            image = pygame.image.load(icon_path).convert_alpha()
        except IOError:
            return None
        _images_cache[name] = image

    if scale_to is None:
        return image

    if scale_type == 'smooth':
        return pygame.transform.smoothscale(image, scale_to)
    else:
        return pygame.transform.scale(image, scale_to)


_cached_font = None


def get_font() -> pygame.font.Font:
    global _cached_font

    if _cached_font is not None:
        return _cached_font

    if FONT_SOURCE == 'file':
        _cached_font = pygame.font.Font(FONT_IDENTIFIER, FONT_SIZE)
    elif FONT_SOURCE == 'system':
        _cached_font = pygame.font.SysFont(FONT_IDENTIFIER, FONT_SIZE)
    elif FONT_SOURCE == 'auto':
        if 'zh' in USER_LOCALE or 'CN' in USER_LOCALE:
            names = [
                'Microsoft YaHei',
                'SimHei',
                'PingFanc SC',
                'WenQuanYi Zen Hei',
                'Noto Sans SC',
                'Sans',
                'sans-serif',
            ]
        else:
            names = ['Sans', 'sans-serif']
        _cached_font = pygame.font.SysFont(names, FONT_SIZE)
    else:
        raise ValueError(
            f'invalid FONT_SOURCE {FONT_SOURCE!r}, allowed values are "file", "system", "auto"'
        )

    return _cached_font
