import abc
import random
from enum import Flag, auto
from typing import TYPE_CHECKING, Any

import numpy as np
import pygame

from .const import DEFAULT_TEMP
from .util import GameSound, blend

if TYPE_CHECKING:
    from .gamemap import GameMap


available_materials = {}


_pre_generated_random = np.array(np.random.random(size=1234), dtype=np.float64)
_random_index = 0


def _fast_choice2(a: Any, b: Any) -> Any:
    global _random_index
    if _random_index >= _pre_generated_random.shape[0]:
        _random_index = 0

    random_value = _pre_generated_random[_random_index]
    _random_index += 1

    return a if random_value > 0.5 else b


def _fast_randint(a: int, b: int) -> int:
    if a > b:
        a, b = b, a

    global _random_index
    if _random_index >= _pre_generated_random.shape[0]:
        _random_index = 0

    random_value = _pre_generated_random[_random_index]
    _random_index += 1

    # Scaling the value to needed range
    result = a + int(random_value * (b - a + 1))

    # Guarantee being in range (needed because of rounding)
    return min(b, max(a, result))


def _fall_gas(this, game_map, x, y):
    for remote_pos in (
        (x, y + 1),
        _fast_choice2((x - 1, y), (x + 1, y)),
        _fast_choice2((x - 1, y + 1), (x + 1, y + 1)),
    ):
        remote_dot = game_map[remote_pos]
        if remote_dot is not None and remote_dot.tags & MaterialTags.SPACE:
            game_map[remote_pos] = this
            game_map[x, y] = remote_dot
            break


def _fall_liquid(this, game_map, x, y):
    remote_dot = game_map[x, y + 1]
    if remote_dot is not None and remote_dot.tags & MaterialTags.BULK:
        game_map[x, y + 1] = this
        game_map[x, y] = remote_dot
        return

    for remote_pos in (
        (x, y - 1),
        _fast_choice2((x - 1, y), (x + 1, y)),
        _fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
    ):
        remote_dot = game_map[remote_pos]
        if remote_dot is not None and remote_dot.tags & MaterialTags.SPARSENESS:
            game_map[remote_pos] = this
            game_map[x, y] = remote_dot
            break


def _fall_sand(this, game_map, x, y):
    for remote_pos in (x, y - 1), _fast_choice2((x - 1, y - 1), (x + 1, y - 1)):
        remote_dot = game_map[remote_pos]

        if remote_dot is not None and remote_dot.tags & MaterialTags.SPARSENESS:
            game_map[remote_pos] = this
            game_map[x, y] = remote_dot
            break


def _von_neumann_hood(game_map, x, y, tags):
    for rx, ry in (1, 0), (-1, 0), (0, 1), (0, -1):
        idx = x + rx, y + ry
        dot = game_map[idx]
        if dot is not None and game_map[idx].tags & tags:
            yield *idx, dot


def _moore_hood(game_map, x, y, tags):
    for rx, ry in (1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1):
        idx = x + rx, y + ry
        dot = game_map[idx]
        if dot is not None and game_map[idx].tags & tags:
            yield *idx, dot


class MaterialTags(Flag):
    NULL = 0
    SOLID = auto()
    BULK = auto()
    LIQUID = auto()
    GAS = auto()
    SPACE = auto()
    SPARSENESS = GAS | SPACE
    MOVABLE = BULK | LIQUID | GAS


class BaseMaterial(abc.ABC):
    '''Abstract base class for materials.'''

    def __init__(self, game_map, x, y):
        pass

    def __init_subclass__(cls, display_name: str = ''):
        super().__init_subclass__()
        if display_name:
            available_materials[display_name] = cls

    @property
    @abc.abstractmethod
    def color(self) -> pygame.Color:
        '''Color of a dot.'''

    temp = DEFAULT_TEMP

    @property
    @abc.abstractmethod
    def heat_capacity(self) -> int:
        '''Heat capacity factor of a dot.'''

    @property
    @abc.abstractmethod
    def thermal_conductivity(self) -> int:
        '''Thermal conductivity factor of a dot.'''

    @property
    @abc.abstractmethod
    def tags(self) -> MaterialTags:
        '''Tags of a dot.'''

    def update(self, game_map: 'GameMap', x: int, y: int) -> None:
        '''Updates physical processes of a dot.'''


class Space(BaseMaterial, display_name='Space'):  # air
    color = pygame.Color(0, 0, 0, 0)
    heat_capacity = 0.3  # moderate, air easily changes temp
    thermal_conductivity = (
        0.001  # we need VERY LOW value so the atmosphere won't immediately heat up
    )
    tags = MaterialTags.SPACE

    @classmethod
    def create(cls):
        '''Pure method to create Space instances without game map or coordinates.'''
        return cls(None, None, None)


class Sand(BaseMaterial, display_name='Sand'):
    heat_capacity = 0.3  # moderate, sand stores some heat
    thermal_conductivity = 0.1  # sand transfers heat slowly
    tags = MaterialTags.BULK

    def __init__(self, game_map, x, y):
        self._is_glass = False
        self._original_sand_color = pygame.Color(0xFF, random.randint(0x99, 0xFF), 0)
        GameSound('material.Sand').play()

    @property
    def color(self):
        t = (self.temp - 400) / (1973 - 400)
        if self._is_glass:
            return blend(pygame.Color("#96947755"), pygame.Color("#FF880085"), t)
        else:
            return blend(self._original_sand_color, pygame.Color("#FF6600AA"), t)

    # @profile(stdout=False, filename='Sand-update.prof')
    def update(self, game_map, x, y):
        if self.temp > 1973:  # 1700 *C, 3092 *F
            if not self._is_glass:
                self._is_glass = True
                GameSound('convert.Sand_to_glass').play()
            _fall_liquid(self, game_map, x, y)

        elif not self._is_glass:
            _fall_sand(self, game_map, x, y)


class Water(BaseMaterial, display_name='Water'):
    heat_capacity = 0.7  # liquids store heat well
    thermal_conductivity = 0.3  # moderate transfer

    def __init__(self, game_map, x, y):
        self._liquid_color = pygame.Color(0, random.randint(0x95, 0xBB), 0x99)

    def update(self, game_map, x, y):
        if self.temp < 273:
            pass  # do nothing
        elif self.temp < 373:
            _fall_liquid(self, game_map, x, y)
        else:
            _fall_gas(self, game_map, x, y)
            # hack to make clouds rain
            if y == game_map.size[1] - 1:
                self.temp -= 10

    @property
    def color(self):
        if self.temp < 273:
            return pygame.Color("#66C8E0B7")
        elif self.temp < 373:
            return self._liquid_color
        else:
            return pygame.Color("#28BBC53D")

    @property
    def tags(self):
        if self.temp < 273:
            return MaterialTags.SOLID
        elif self.temp < 373:
            return MaterialTags.LIQUID
        else:
            return MaterialTags.GAS


class UnbreakableWall(BaseMaterial, display_name='Unbreakable Wall'):
    color = pygame.Color(0xFF, 0xFF, 0xFF)
    heat_capacity = 0  # does not change its temp
    thermal_conductivity = 0  # does not transfer heat
    tags = MaterialTags.SOLID

    def update(self, game_map, x, y):
        # Unbreakable wall does not change
        pass


class Lava(BaseMaterial, display_name='Lava'):
    heat_capacity = 0.2  # hot, but doesn't store much
    thermal_conductivity = 0.8  # transfers heat well
    temp = 1200  # 927 *C, 1700 *F

    def __init__(self, game_map, x, y):
        GameSound('material.Lava').play()

    def update(self, game_map, x, y):
        if self.temp > 400:  # 127 *C, 260 *F
            _fall_liquid(self, game_map, x, y)

    @property
    def color(self):
        mid_temp = self.temp - 1200
        red = mid_temp / 5 + 0xFF
        green = mid_temp / 8 + 0x10
        return pygame.Color(max(0x44, min(255, red)), max(0, min(255, green)), 0)

    @property
    def tags(self):
        return MaterialTags.LIQUID if self.temp > 400 else MaterialTags.SOLID


class Plus100K(BaseMaterial, display_name='+100 K'):
    color = None
    heat_capacity = None
    thermal_conductivity = None
    tags = MaterialTags.NULL

    def __new__(cls, game_map, x, y):
        dot = game_map[x, y]
        if dot is not None:
            dot.temp += 100
        return dot


class Minus100K(BaseMaterial, display_name='-100 K'):
    color = None
    heat_capacity = None
    thermal_conductivity = None
    tags = MaterialTags.NULL

    def __new__(cls, game_map, x, y):
        dot = game_map[x, y]
        if dot is not None:
            dot.temp -= 100
        return dot


class BlackHole(BaseMaterial, display_name='Black Hole'):
    color = pygame.Color("#1F1F1F")
    heat_capacity = 0
    thermal_conductivity = 0
    tags = MaterialTags.SOLID

    def update(self, game_map, x, y):
        for rx, ry, _ in _von_neumann_hood(game_map, x, y, MaterialTags.MOVABLE):
            game_map[rx, ry] = Space(game_map, rx, ry)


class Tap(BaseMaterial, display_name='Tap'):
    color = pygame.Color("#67a046")
    heat_capacity = 0.2  # factors like Lava got
    thermal_conductivity = 0.6
    tags = MaterialTags.SOLID

    def __init__(self, game_map, x, y):
        self._generate_type = None

    def update(self, game_map, x, y):
        if self._generate_type is None:
            for rx, ry, dot in _von_neumann_hood(game_map, x, y, MaterialTags.MOVABLE):
                self._generate_type = type(dot)
                break
        elif _fast_randint(1, 6) == 6:
            for rx, ry, _ in _von_neumann_hood(game_map, x, y, MaterialTags.SPACE):
                game_map[rx, ry] = self._generate_type(game_map, rx, ry)
        elif _fast_randint(1, 30) == 16:  # exchange tap's generate type to other taps
            for _, _, dot in _moore_hood(game_map, x, y, MaterialTags.SOLID):
                if hasattr(dot, '_generate_type'):  # we could create a separate tag for taps
                    dot._generate_type = self._generate_type


class Propane(BaseMaterial, display_name='Propane'):
    color = pygame.Color("#385DA345")
    heat_capacity = 0.3  # factors like Space got
    thermal_conductivity = 0.5
    tags = MaterialTags.GAS

    def update(self, game_map, x, y):
        if self.temp > 700:  # ~500 *C
            game_map[x, y] = Fire(game_map, x, y)
            for rx, ry, _ in _von_neumann_hood(game_map, x, y, MaterialTags.GAS):
                game_map[rx, ry] = Fire(game_map, rx, ry)

        _fall_gas(self, game_map, x, y)


class Fire(BaseMaterial, display_name='Fire'):
    heat_capacity = 0  # fire cannot store heat
    thermal_conductivity = 1  # very high
    tags = MaterialTags.GAS

    def __init__(self, game_map, x, y):
        self.temp = _fast_randint(800, 1200)  # near 600..1000*C

    def update(self, game_map, x, y):
        self.temp -= 20

        if self.temp <= 800:
            game_map[x, y] = Space(game_map, x, y)
            return

        _fall_gas(self, game_map, x, y)

    @property
    def color(self):
        factor = (self.temp - 800) / (1200 - 800)
        return blend(pygame.Color('#ff000044'), pygame.Color("#FFff00"), factor)
