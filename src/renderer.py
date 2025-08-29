from io import BytesIO
from pathlib import Path
import shutil
from typing import Callable

import pygame

from .config import (
    DEFAULT_TEMP,
    CAPTURE_PATH_FACTORY,
    SCREENSHOT_PATH_FACTORY,
    SCREENSHOT_DOT_SIZE,
    SCREENSHOT_TYPE_HINT,
)
from .gamemap import GameMap
from .materials import BaseMaterial
from .util import blend

type RenderMask = Callable[[BaseMaterial], pygame.Color]
available_render_masks = []


class Renderer:
    '''Runs rendering a physical model onto a surface.'''

    def __init__(self, game_map: GameMap, dest: pygame.Surface, bg_color: pygame.Color):
        self._map = game_map
        self._dest = dest
        self._clock = pygame.time.Clock()
        self._temp_surface = pygame.Surface((0, 0))

        self._is_capturing = False
        self._capture_path = None
        self._frame_idx = 0

        self._render_mask = available_render_masks[0]
        self._render_mask_idx = 0

        self.is_paused = False
        self.bg_color = bg_color

    def render(
        self,
        visible_area: pygame.Rect,
        framerate: float = 0,
    ) -> None:
        if self.is_paused:
            return

        # We will not iterate through the whole visible area, only its part that is inside the map.
        # Unless we are capturing, then we render the whole map.
        if self._is_capturing:
            x_start, y_start = 0, 0
            x_end, y_end = self._map.size
        else:
            x_start = max(visible_area.left, 0)
            x_end = min(self._map.size[0], visible_area.right)
            y_start = max(visible_area.top, 0)
            y_end = min(self._map.size[1], visible_area.bottom)

        inner_w = x_end - x_start
        inner_h = y_end - y_start

        if inner_w <= 0 or inner_h <= 0:
            self._clock.tick(framerate)
            return

        # Create or resize the temp surface for the inner area only
        if self._temp_surface.get_size() != (inner_w, inner_h):
            self._temp_surface = pygame.Surface((inner_w, inner_h))
        self._temp_surface.fill(self.bg_color)  # Fill with background color

        # Iterating through the map
        with pygame.PixelArray(self._temp_surface) as temp_array:
            view = self._map.get_view()

            for x in range(x_start, x_end):
                for y in range(y_start, y_end):
                    dot = view[x, self._map.invy(y)]
                    original_color = self._render_mask(dot)

                    if original_color.a == 255:
                        temp_array[x - x_start, y - y_start] = original_color
                    elif original_color.a > 0:
                        temp_array[x - x_start, y - y_start] = blend(self.bg_color, original_color)

        # Calculate where to blit the scaled inner area
        screen_w, screen_h = self._dest.get_size()
        vis_w, vis_h = visible_area.size
        scale_x = screen_w / vis_w
        scale_y = screen_h / vis_h

        # The position of the inner area in the visible area (in pixels)
        inner_left = (x_start - visible_area.left) * scale_x
        inner_top = (y_start - visible_area.top) * scale_y
        inner_right = (x_end - visible_area.left) * scale_x
        inner_bottom = (y_end - visible_area.top) * scale_y
        blit_rect = pygame.Rect(
            int(inner_left),
            int(inner_top),
            int(inner_right - inner_left),
            int(inner_bottom - inner_top),
        )

        # If capturing, save the frame
        if self._is_capturing:
            capture_path = self._capture_path / f'frame_{self._frame_idx:06d}.png'
            pygame.image.save(self._temp_surface, capture_path)
            self._frame_idx += 1

        # Scale the inner temp surface to the correct size and blit
        scaled_inner = pygame.transform.scale(self._temp_surface, blit_rect.size)
        self._dest.blit(scaled_inner, blit_rect.topleft)
        self._clock.tick(framerate)

    def get_fps(self) -> float:
        return self._clock.get_fps()

    def begin_capturing(self) -> None:
        self._capture_path = Path(CAPTURE_PATH_FACTORY())
        if self._capture_path.is_file():  # also check for existing
            self._capture_path.unlink(missing_ok=False)
        elif self._capture_path.is_dir():  # same
            shutil.rmtree(self._capture_path, ignore_errors=False)

        self._capture_path.mkdir(parents=True, exist_ok=False)
        self._is_capturing = True
        self._frame_idx = 0

    def end_capturing(self) -> None:
        self._is_capturing = False

    def is_capturing(self) -> bool:
        return self._is_capturing

    def take_screenshot(self) -> None:
        if self._temp_surface.size == self._map.size:
            # Then just use already rendered surface
            unscaled_surf = self._temp_surface
        else:
            unscaled_surf = pygame.Surface(self._map.size)
            with pygame.PixelArray(unscaled_surf) as temp_array:
                for x in range(self._map.size[0]):
                    for y in range(self._map.size[1]):
                        dot = self._map[x, self._map.invy(y)]
                        original_color = self._render_mask(dot)

                        if original_color.a == 255:
                            temp_array[x, y] = original_color
                        elif original_color.a > 0:
                            temp_array[x, y] = blend(self.bg_color, original_color)

        size = (
            unscaled_surf.size[0] * SCREENSHOT_DOT_SIZE[0],
            unscaled_surf.size[1] * SCREENSHOT_DOT_SIZE[1],
        )
        scaled_surf = pygame.transform.scale(unscaled_surf, size)

        with open(SCREENSHOT_PATH_FACTORY(), 'wb') as file:
            pygame.image.save(scaled_surf, file, SCREENSHOT_TYPE_HINT)

    def next_render_mask(self) -> None:
        self._render_mask_idx = (self._render_mask_idx + 1) % len(available_render_masks)
        self._render_mask = available_render_masks[self._render_mask_idx]


def add_render_mask(func: RenderMask | None = None):
    def decorate(func):
        available_render_masks.append(func)

    if func is None:
        return decorate
    else:
        decorate(func)


@add_render_mask
def _normal(dot):
    return dot.color


@add_render_mask
def _thermal(dot):
    darkscale = (dot.color.r + dot.color.g + dot.color.b) // 6  # 50 % of grayscale
    temp_factor = dot.temp / 500

    red = min(0xFF, darkscale + temp_factor * 0xBF)
    green = min(0xFF, max(0x00, darkscale + (temp_factor - 1) * 0x3F))

    return pygame.Color(red, green, darkscale)
