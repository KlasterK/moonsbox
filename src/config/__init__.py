import toml
import pygame
from pathlib import Path
import locale
import time
from ._defaults import ConfigDefaults


# Helper functions for type casting
def parse_color(val):
    if isinstance(val, pygame.Color):
        return val
    return pygame.Color(val)


def parse_rect(val):
    if isinstance(val, pygame.Rect):
        return val
    if isinstance(val, (list, tuple)) and len(val) == 4:
        return pygame.Rect(*val)
    if isinstance(val, str):
        parts = [int(x) for x in val.replace('[', '').replace(']', '').split(',')]
        return pygame.Rect(*parts)
    raise ValueError(f"Invalid rect: {val}")


def parse_tuple(val):
    if isinstance(val, (list, tuple)):
        return tuple(val)
    if isinstance(val, str):
        return tuple(int(x) for x in val.replace('[', '').replace(']', '').split(','))
    raise ValueError(f"Invalid tuple: {val}")


def parse_path(val):
    return Path(val)


def get_locale(val):
    if val == "auto":
        return locale.getdefaultlocale()[0]
    return val


def get_path_factory(val):
    if isinstance(val, str):
        return lambda: time.strftime(val)
    raise ValueError(f"Invalid path factory: {val}")


# Load user config
def load_config():
    defaults = ConfigDefaults()
    try:
        user_cfg = toml.load('user/config.toml')
    except Exception:
        user_cfg = {}

    def get(key, default):
        val = user_cfg.get(key, default)
        # Type casting
        if key in [
            "MAP_INNER_COLOR",
            "MAP_OUTER_COLOR",
            "PALETTE_SELECTION_OUTER_COLOR",
            "PALETTE_SELECTION_INNER_COLOR",
            "PALETTE_SHADOW_COLOR",
            "DEBUG_COLOR",
        ]:
            try:
                return parse_color(val)
            except Exception:
                return default
        if key in ["SCREEN_SIZE", "MAP_SIZE", "PALETTE_ICON_SIZE", "SCREENSHOT_DOT_SIZE"]:
            try:
                return parse_tuple(val)
            except Exception:
                return default
        if key == "VISIBLE_AREA":
            try:
                return parse_rect(val)
            except Exception:
                return default
        if key == "ASSETS_ROOT":
            try:
                return parse_path(val)
            except Exception:
                return default
        if key == "USER_LOCALE":
            return get_locale(val)
        if key in ("SCREENSHOT_PATH_FACTORY", 'CAPTURE_PATH_FACTORY'):
            return get_path_factory(val)
        return val

    # Build config namespace
    config = {}
    for field in defaults.__dataclass_fields__:
        config[field] = get(field, getattr(defaults, field))
    return config


_config = load_config()

# Expose config as module-level variables
for k, v in _config.items():
    globals()[k] = v
