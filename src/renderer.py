from typing import Callable

import pygame

from . import blend
from .gamemap import GameMap
from .materials import BaseMaterial

type RenderMask = Callable[[BaseMaterial], pygame.Color]


class Renderer:
    '''Runs rendering a physical model onto a surface.'''

    def __init__(self, game_map: GameMap, dest: pygame.Surface):
        self._map = game_map
        self._dest = dest
        self._clock = pygame.time.Clock()
        self._temp_surface = pygame.Surface((0, 0))

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

        # Iterating through the map
        for x in range(x_start, x_end):
            for y in range(y_start, y_end):
                dot = self._map[x, self._map.invy(y)]
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

        # Scale the inner temp surface to the correct size and blit
        scaled_inner = pygame.transform.scale(self._temp_surface, blit_rect.size)
        self._dest.blit(scaled_inner, blit_rect.topleft)
        self._clock.tick(framerate)

    def get_fps(self) -> float:
        return self._clock.get_fps()


# class Renderer:
#     '''Runs rendering a physical model onto a surface.'''

#     def __init__(self, game_map: GameMap, dest: pygame.Surface):
#         self._map = game_map
#         self._dest = dest
#         self._render_surface = pygame.Surface(game_map.size)

#     def render(self, visible_area: pygame.Rect, mask: RenderMask, bg: pygame.Color):
#         w, h = self._map.size

#         left = max(0, visible_area.left)
#         top = max(0, visible_area.top)

#         # Размеры области:
#         width = min(w - left, visible_area.width)
#         height = min(h - top, visible_area.height)

#         render_area = pygame.Rect(left, top, width, height)
#         # render_area = pygame.Rect(
#         #     max(0, visible_area.left),
#         #     max(0, visible_area.top),
#         #     min(w, visible_area.left + w),
#         #     min(h, visible_area.top + h),
#         # )
#         array = pygame.PixelArray(self._render_surface)

#         for x in range(render_area.left, render_area.right):
#             for y in range(render_area.top, render_area.bottom):
#                 dot = self._map[x, y, '~y']
#                 if dot is not None:
#                     array[x, y] = mask(dot)

#         sub = self._render_surface.subsurface(render_area)
#         sub_scaled = pygame.transform.scale(sub, visible_area.size)
#         self._dest.fill(bg)
#         self._dest.blit(sub_scaled, visible_area.topleft)
