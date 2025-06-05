import numba
import pygame

from .gamemap import GameMap


class SimulationManager:
    '''Runs physical processes of the game map.'''

    @numba.jit(forceobj=True)
    def __init__(self, game_map: GameMap):
        self._map = game_map
        self._clock = pygame.Clock()

    # @profile(stdout=False, filename='SimulationManager-tick.prof')
    # @numba.jit(forceobj=True)
    def tick(self, framerate: float = 0) -> None:
        '''Updates the map.'''

        size_x = self._map.size[0]
        size_y = self._map.size[1]

        # Exchanging temp of neighbours
        for x in range(size_x):
            for y in range(size_y):
                dot = self._map[x, y]

                for neighbour in (
                    self._map[x - 1, y],
                    self._map[x + 1, y],
                    self._map[x, y - 1],
                    self._map[x, y + 1],
                ):
                    if neighbour is not None:
                        dot.temp += (
                            (neighbour.temp - dot.temp)
                            * neighbour.thermal_conductivity
                            * dot.heat_capacity
                        )

                dot.update(self._map, (x, y))

        self._clock.tick(framerate)

    def get_tps(self):
        return self._clock.get_fps()
