import numpy as np
import pygame

from .config import TEMP_IS_EXCHANGING
from .gamemap import GameMap
from .materials import MaterialTags


class SimulationManager:
    def __init__(self, game_map: GameMap):
        self._map = game_map
        self._clock = pygame.Clock()
        self._size_x, self._size_y = game_map.size

    def tick(self, framerate: float = 0):
        view = self._map.get_view()
        size_x, size_y = self._map.size

        if TEMP_IS_EXCHANGING:
            for x in range(size_x):
                for y in range(size_y):
                    dot = view[x, y]
                    temp_delta = 0.0

                    for dx, dy in ((0, 1), (1, 0), (-1, 0), (0, -1)):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < size_x and 0 <= ny < size_y:
                            nbr = view[nx, ny]
                            temp_delta += (
                                (nbr.temp - dot.temp)
                                * min(nbr.thermal_conductivity, dot.thermal_conductivity)
                                * (1 - dot.heat_capacity)
                            )

                    dot.temp += temp_delta / 4
                    if dot.temp < 0:
                        dot.temp = 0

        for x in range(size_x):
            for y in range(size_y):
                dot = view[x, y]
                if not dot.tags & MaterialTags.GAS:
                    dot.update(x, y)

        # We need to update gases separately, because the updating is going upwards,
        # and gas with y=0 will raise, then the same gas, now with y=1 will raise,
        # and at the end it is on the top.
        tags_map = np.array([[dot.tags.value for dot in row] for row in view], dtype=np.uint8)
        # Create mask of gases
        gas_mask = tags_map & MaterialTags.GAS.value
        # Getting indices of gas dots
        x_coords, y_coords = np.where(gas_mask)
        sorted_indices = np.argsort(-y_coords)  # Reversing downwards the indices

        for idx in sorted_indices:
            x, y = x_coords[idx], y_coords[idx]
            view[x, y].update(x, y)

        self._clock.tick(framerate)

    def get_tps(self):
        return self._clock.get_fps()
