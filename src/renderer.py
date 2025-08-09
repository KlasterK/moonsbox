from pathlib import Path
import shutil
from typing import Callable

import pygame

from .config import DEFAULT_TEMP, CAPTURE_PATH_FACTORY
from .gamemap import GameMap
from .materials import BaseMaterial
from .util import blend

type RenderMask = Callable[[BaseMaterial], pygame.Color]


class Renderer:
    '''Runs rendering a physical model onto a surface.'''

    def __init__(self, game_map: GameMap, dest: pygame.Surface):
        self._map = game_map
        self._dest = dest
        self._clock = pygame.time.Clock()
        self._temp_surface = pygame.Surface((0, 0))
        self._is_capturing = False
        self._capture_path = None
        self._frame_idx = 0

    # @profile(stdout=False, filename='Renderer-render.prof')
    def render(
        self,
        visible_area: pygame.Rect,
        mask: RenderMask,
        bg: pygame.Color,
        framerate: float = 0,
    ) -> None:
        # We will not iterate through the whole visible area, only its part that is inside the map.
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
        self._temp_surface.fill(bg)  # Fill with background color

        # Creating a pixel array to do fast pixel manipulation
        temp_array = pygame.PixelArray(self._temp_surface)

        view = self._map.get_view()

        # Iterating through the map
        for x in range(x_start, x_end):
            for y in range(y_start, y_end):
                dot = view[x, self._map.invy(y)]
                original_color = mask(dot)  # mask is a function returning color for the dot
                if original_color.a == 255:
                    temp_array[x - x_start, y - y_start] = original_color
                elif original_color.a > 0:
                    temp_array[x - x_start, y - y_start] = blend(bg, original_color)

        # Unlocking the surface
        temp_array.close()

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
        # TODO: render the whole map when capturing even if the visible area is too small
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

    # more explicit API than @property
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


available_render_masks = []


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
    # Simple thermal imagination, where R=temp, G=B=grayscale
    # TODO: implement more complex and handy thermal rendering
    grayscale = (dot.color.r + dot.color.g + dot.color.b) / 3
    red = max(0, min(255, dot.temp - DEFAULT_TEMP + 0x7F))
    return pygame.Color(red, grayscale, grayscale)
