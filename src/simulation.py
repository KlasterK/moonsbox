import numpy as np
import pygame

from .config import TEMP_IS_EXCHANGING
from .gamemap import GameMap
from .materials import BaseMaterial, MaterialTags


class SimulationManager:
    '''Runs physical processes of the game map.'''

    def __init__(self, game_map: GameMap):
        self._map = game_map
        self._clock = pygame.Clock()

    def tick(self, framerate: float = 0) -> None:
        '''Updates the map.'''

        size_x = self._map.size[0]
        size_y = self._map.size[1]

        view = self._map.get_view()

        # Exchanging temp of neighbours
        for x in range(size_x):
            dot_below = None
            for y in range(size_y):
                dot: BaseMaterial = view[x, y]

                if TEMP_IS_EXCHANGING:
                    if dot_below is not None:
                        dot.temp += (
                            (dot_below.temp - dot.temp)
                            * dot_below.thermal_conductivity
                            * dot.heat_capacity
                        )

                    if x > 0:
                        neighbour = view[x - 1, y]
                        dot.temp += (
                            (neighbour.temp - dot.temp)
                            * neighbour.thermal_conductivity
                            * dot.heat_capacity
                        )

                    if x < size_x - 1:
                        neighbour = view[x + 1, y]
                        dot.temp += (
                            (neighbour.temp - dot.temp)
                            * neighbour.thermal_conductivity
                            * dot.heat_capacity
                        )

                    if y < size_y - 1:
                        neighbour = view[x, y + 1]
                        dot.temp += (
                            (neighbour.temp - dot.temp)
                            * neighbour.thermal_conductivity
                            * dot.heat_capacity
                        )

                dot_below = dot
                if not dot.tags & MaterialTags.GAS:
                    dot.update(self._map, x, y)

        # We need to update gases separately, because the updating is going upwards,
        # and gas with y=0 will raise, then the same gas, now with y=1 will raise,
        # and it is on the top.
        tags_map = np.array([[dot.tags.value for dot in row] for row in view], dtype=np.uint8)
        # Create mask of gases
        gas_mask = tags_map & MaterialTags.GAS.value
        # Getting indices of gas dots
        x_coords, y_coords = np.where(gas_mask)
        sorted_indices = np.argsort(-y_coords)  # Reversing downwards the indices

        for idx in sorted_indices:
            x, y = x_coords[idx], y_coords[idx]
            view[x, y].update(self._map, x, y)

        self._clock.tick(framerate)

    def get_tps(self):
        return self._clock.get_fps()
