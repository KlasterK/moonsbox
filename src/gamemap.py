from typing import Callable

import numpy as np
import PIL.Image
import pygame

from . import DRAWING_IS_CIRCULAR
from .materials import BaseMaterial, Space

type MFactory = Callable[['GameMap', tuple[int, int]], BaseMaterial] | type[BaseMaterial]


class GameMap:
    '''Represents physical model of the game map.'''

    def __init__(self, size: tuple[int, int]):
        self.size = size
        self._array = np.full(size, Space(None, None), dtype=np.object_)

    def __getitem__(self, pos: tuple[int, int]) -> BaseMaterial | None:
        '''Returns a dot at the given position or None if the position is out of bounds.'''

        # NOTE: in numpy if you using negative index, you will get the value from the end
        if pos[0] >= 0 and pos[0] < self.size[0] and pos[1] >= 0 and pos[1] < self.size[1]:
            return self._array[pos]
        else:
            return None

    def __setitem__(self, pos: tuple[int, int], value: BaseMaterial) -> None:
        '''Sets a dot at the given position if the position is out of bounds, noexcept.'''

        if pos[0] >= 0 and pos[0] < self.size[0] and pos[1] >= 0 and pos[1] < self.size[1]:
            self._array[pos] = value

    def get_view(self) -> np.ndarray:
        '''Returns a view of internal numpy array.'''

        return self._array.view()

    def invy(self, y: int) -> int:
        '''Inverts Y axis for the given value.'''

        return self.size[1] - 1 - y

    def invy_pos(self, pos: tuple[int, int]) -> tuple[int, int]:
        '''Inverts Y axis for the given position.'''

        return pos[0], self.size[1] - 1 - pos[1]

    def bounds(self, pos: tuple[int, int]) -> bool:
        '''Checks if the pos is in the map bounds.'''

        return pos[0] >= 0 and pos[0] < self.size[0] and pos[1] >= 0 and pos[1] < self.size[1]

    def resize(self, new_size: tuple[int, int]) -> None:
        '''Resizes the map.'''

        self.size = new_size
        self._array.resize(new_size)

    def fill(self, material_factory: MFactory):
        '''Fills the map with the given material.'''

        for x in range(self.size[0]):
            for y in range(self.size[1]):
                self._array[x, y] = material_factory(self, (x, y))

    def draw_rect(self, area: pygame.Rect, material_factory: MFactory) -> None:
        '''Fills a rectangle on the map with the given material.'''

        x_start = max(area.left, 0)
        x_end = min(area.right, self.size[0])

        y_start = max(area.top, 0)
        y_end = min(area.bottom, self.size[1])

        for x in range(x_start, x_end):
            for y in range(y_start, y_end):
                self._array[x, y] = material_factory(self, (x, y))

    def draw_ellipse(self, area: pygame.Rect, material_factory: MFactory) -> None:
        '''Draws an ellipse on the map with the given material.'''

        rows, cols = self.size

        # Creating coordinate grids
        x_grid, y_grid = np.ogrid[:rows, :cols]

        # Getting the center and half-axises
        x0, y0 = area.center
        a = area.width / 2
        b = area.height / 2

        # Evaluating ellipse equation ((x - x0)/a)^2 + ((y - y0)/b)^2 <= 1
        mask = ((x_grid - x0) / a) ** 2 + ((y_grid - y0) / b) ** 2 <= 1

        # Filling the map with the material
        for x in range(rows):
            for y in range(cols):
                if mask[x, y]:
                    self._array[x, y] = material_factory(self, (x, y))

    def draw_line(
        self,
        start: tuple[int, int],
        end: tuple[int, int],
        width: int,
        material_factory: MFactory,
    ) -> None:
        '''Draws a line with the given width on the map with the given material.'''

        delta_x = abs(start[0] - end[0])
        delta_y = abs(start[1] - end[1])

        current_x = start[0]
        current_y = start[1]

        step_x = 1 if start[0] < end[0] else -1
        step_y = 1 if start[1] < end[1] else -1

        points = []
        # Bresenham's algorithm to collect all points along the line
        if delta_x > delta_y:
            error = delta_x / 2.0

            while current_x != end[0]:
                points.append((current_x, current_y))

                current_x += step_x

                error -= delta_y
                if error < 0:
                    current_y += step_y
                    error += delta_x

            points.append((current_x, current_y))
        else:
            error = delta_y / 2.0

            while current_y != end[1]:
                points.append((current_x, current_y))

                current_y += step_y

                error -= delta_x
                if error < 0:
                    current_x += step_x
                    error += delta_y

            points.append((current_x, current_y))

        # Draw width-wide line by filling a square or disk around each point
        radius = max(0, width // 2)
        for px, py in points:
            for dx in range(-radius, radius + 1):
                for dy in range(-radius, radius + 1):
                    tx, ty = px + dx, py + dy
                    if not self.bounds((tx, ty)):
                        continue
                    if DRAWING_IS_CIRCULAR:
                        # Only fill points inside the circle
                        if dx * dx + dy * dy > radius * radius:
                            continue
                    self._array[tx, ty] = material_factory(self, (tx, ty))

    def take_screenshot(self) -> PIL.Image.Image:
        # PIL expecting shape (y, x, channels)
        w, h = self.size
        image_buffer = np.ndarray((h, w, 4), dtype=np.uint8)

        # for i, dot in enumerate(self._old.flat):
        #     image_buffer[i * 4 + 0] = dot.color.r
        #     image_buffer[i * 4 + 1] = dot.color.g
        #     image_buffer[i * 4 + 2] = dot.color.b
        #     image_buffer[i * 4 + 3] = dot.color.a

        for x in range(w):
            for y in range(h):
                color = self._array[x, self.invy(y)].color

                image_buffer[y, x, 0] = color.r
                image_buffer[y, x, 1] = color.g
                image_buffer[y, x, 2] = color.b
                image_buffer[y, x, 3] = color.a

        return PIL.Image.fromarray(image_buffer, 'RGBA')
