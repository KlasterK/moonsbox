import numpy as np
import pygame

from .gamemap import GameMap


class SimulationManager:
    '''Runs physical processes of the game map.'''

    def __init__(self, game_map: GameMap):
        self._map = game_map
        self._clock = pygame.Clock()

    def tick(self, framerate: float = 0) -> None:
        '''Updates the map.'''

        size_x = self._map.size[0]
        size_y = self._map.size[1]
        # upper_line = self._map

        view = self._map.get_view()

        # Exchanging temp of neighbours
        for x in range(size_x):
            dot_below = None
            for y in range(size_y):
                dot = view[x, y]

                # for neighbour in (
                #     view[x - 1, y],
                #     view[x + 1, y],
                #     dot_below,
                #     view[x, y + 1],
                # ):

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
                dot.update(self._map, (x, y))

        # np.apply_over_axes()

        self._clock.tick(framerate)

    def get_tps(self):
        return self._clock.get_fps()
