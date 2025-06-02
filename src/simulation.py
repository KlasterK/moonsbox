import pygame

from .gamemap import GameMap


class SimulationManager:
    '''Runs physical processes of the game map.'''

    def __init__(self, game_map: GameMap):
        self._map = game_map
        self._clock = pygame.Clock()

    def tick(self, framerate: float = 0) -> None:
        '''Updates the map.'''

        size_x, size_y = self._map.size

        # Exchanging temp of neighbours
        for x in range(size_x):
            for y in range(size_y):
                cell = self._map[x, y]

                # Collecting neighbours
                neighbors = []
                for nb_relx, nb_rely in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    nb_x, nb_y = x + nb_relx, y + nb_rely

                    if 0 <= nb_x < size_x and 0 <= nb_y < size_y:
                        neighbor = self._map[nb_x, nb_y]
                        neighbors.append(neighbor)

                if not neighbors:
                    continue

                # Average temp
                avg_temp = sum(n.temp for n in neighbors) / len(neighbors)

                # Assigning temp
                delta = (avg_temp - cell.temp) * cell.thermal_conductivity
                cell.temp += delta * (1 - cell.heat_capacity)

                cell.update(self._map, (x, y))

        self._clock.tick(framerate)

    def get_tps(self):
        return self._clock.get_fps()
