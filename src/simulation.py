import numpy as np
import pygame

from .gamemap import GameMap
from .materials import MaterialTags, PhysicalEntity
from .util import fast_random, fast_randint, fast_choice2


class SimulationManager:
    def __init__(self, game_map: GameMap):
        self._map = game_map
        self._clock = pygame.Clock()
        self._size_x, self._size_y = game_map.size
        self._view = None  # inited in update()

    def get_tps(self):
        return self._clock.get_fps()

    def tick(self, framerate: float = 0):
        self._view = self._map.get_view()
        self._exchange_temp()
        self._update_all()
        self._process_physics()
        self._clock.tick(framerate)

    def _exchange_temp(self):
        size_x, size_y = self._map.size

        for (x, y), dot in np.ndenumerate(self._view):
            temp_delta = 0.0

            for dx, dy in ((0, 1), (1, 0), (-1, 0), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < size_x and 0 <= ny < size_y:
                    nbr = self._view[nx, ny]
                    temp_delta += (
                        (nbr.temp - dot.temp)
                        * min(nbr.thermal_conductivity, dot.thermal_conductivity)
                        * (1 - dot.heat_capacity)
                    )

            dot.temp += temp_delta / 4
            if dot.temp < 0:
                dot.temp = 0

    def _update_all(self):
        for (x, y), dot in np.ndenumerate(self._view):
            dot.update(x, y)

    def _process_physics(self):
        entities_map = np.array(
            [[dot.physical_entity.value for dot in row] for row in self._view], dtype=np.uint8
        )
        for entity, func, inverted in (
            (PhysicalEntity.SAND, self._fall_sand, False),
            (PhysicalEntity.LIQUID, self._fall_liquid, False),
            (PhysicalEntity.LIGHT_GAS, self._fall_light_gas, True),
            (PhysicalEntity.HEAVY_GAS, self._fall_heavy_gas, False),
        ):
            mask = entities_map == entity.value
            # 1D arrays of X and Y coords of needed dots
            x_coords, y_coords = np.where(mask)

            if not inverted:
                indices = range(y_coords.shape[0])
            else:
                # Gas needed to invert Y coords
                indices = range(y_coords.shape[0] - 1, -1, -1)

            for idx in indices:
                x, y = x_coords[idx], y_coords[idx]
                func(x, y)

    def _diffuse(self, central_dot, x, y, tags, chance=0.01):
        # Swaps the dot with some Von Neumann neighbour with a random chance

        if fast_random() > chance:
            return

        nb_pos = ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))[fast_randint(0, 3)]

        if not self._map.bounds(nb_pos):
            return
        nb_dot = self._view[nb_pos]

        if nb_dot.tags & tags:
            self._view[nb_pos] = central_dot
            self._view[x, y] = nb_dot

    def _try_swap(self, src_dot, src_x, src_y, dst_x, dst_y, required_tags) -> bool:
        # If dest dot has required tags, then swap it with source dot and return True
        # Otherwise do nothing and return False

        if not self._map.bounds((dst_x, dst_y)):
            return False

        dst_dot = self._view[dst_x, dst_y]

        if dst_dot.tags & required_tags:
            self._view[dst_x, dst_y] = src_dot
            self._view[src_x, src_y] = dst_dot
            return True
        return False

    def _generic_move(self, x, y, positions, move_tags, diffuse_tags=MaterialTags.NULL):
        # Generic move function

        dot = self._view[x, y]

        for pos in positions:
            if self._try_swap(dot, x, y, *pos, move_tags):
                return

        self._diffuse(dot, x, y, diffuse_tags)

    def _fall_light_gas(self, x, y):
        positions = [
            (x, y + 1),
            fast_choice2((x - 1, y), (x + 1, y)),
            fast_choice2((x - 1, y + 1), (x + 1, y + 1)),
        ]
        self._generic_move(x, y, positions, MaterialTags.SPACE, MaterialTags.GAS)

    def _fall_heavy_gas(self, x, y):
        positions = [
            (x, y + 1),
            (x + 1, y),
            (x - 1, y),
            (x, y - 1),
            (x + 1, y + 1),
            (x + 1, y - 1),
            (x - 1, y + 1),
            (x - 1, y - 1),
        ]
        np.random.shuffle(positions)
        self._generic_move(x, y, positions, MaterialTags.SPACE, MaterialTags.GAS)

    def _fall_liquid(self, x, y):
        positions = [
            (x, y - 1),
            fast_choice2((x - 1, y), (x + 1, y)),
            fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
        ]
        self._generic_move(x, y, positions, MaterialTags.SPARSENESS, MaterialTags.LIQUID)

    def _fall_sand(self, x, y):
        positions = [
            (x, y - 1),
            fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
        ]
        self._generic_move(x, y, positions, MaterialTags.FLOWABLE)
