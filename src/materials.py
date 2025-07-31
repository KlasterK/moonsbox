import abc
import random
from enum import Flag, auto
from typing import TYPE_CHECKING, Any

import numpy as np
import pygame

from .const import DEFAULT_TEMP
from .util import GameSound

if TYPE_CHECKING:
    from .gamemap import GameMap


available_materials = {}


_pre_generated_choice2 = np.random.choice((np.True_, np.False_), size=1234)
_choice2_index = -1


def _fast_choice2(a: Any, b: Any) -> Any:
    # return a if random.randint(0, 1) else b
    global _choice2_index
    if _choice2_index >= 1234:
        _choice2_index = 0

    random_value = _pre_generated_choice2[_choice2_index]
    _choice2_index += 1

    if random_value:
        return a
    else:
        return b


_last_material_flag_index = -1


def _unique_material_flag() -> int:
    # returns a unique bit flag for a material
    global _last_material_flag_index
    _last_material_flag_index += 1
    return 1 << _last_material_flag_index


# increases TPS by 2 ticks
def _fast_isinstance(obj: Any, cls: type['BaseMaterial']) -> bool:
    try:
        return obj.MATERIAL_FLAGS & cls.SINGLETON_MATERIAL_FLAG
    except AttributeError:
        return False


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

    def __new__(cls, game_map, pos: tuple[int, int] | None):
        return super().__new__(cls)

    def __init__(self, game_map, pos: tuple[int, int] | None):
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

    def update(self, game_map: 'GameMap', pos: tuple[int, int]) -> None:
        '''Updates physical processes of a dot.'''


class Space(BaseMaterial, display_name='Space'):  # air
    color = pygame.Color(0, 0, 0, 0)
    heat_capacity = 0.3  # moderate, air easily changes temp
    thermal_conductivity = 0.01  # low but not 0, air transfers heat slowly
    tags = MaterialTags.SPACE


class Sand(BaseMaterial, display_name='Sand'):
    heat_capacity = 0.3  # moderate, sand stores some heat
    thermal_conductivity = 0.1  # sand transfers heat slowly
    tags = MaterialTags.BULK

    def __init__(self, game_map, pos):
        self._is_glass = False
        self._original_sand_color = pygame.Color(0xFF, random.randint(0x99, 0xFF), 0)
        GameSound('material.Sand').play()

    @property
    def color(self):
        if self._is_glass or self.temp >= 1973:
            return pygame.Color("#94967755")
        elif self.temp <= 400:
            return self._original_sand_color
        else:
            # Linear blend between sand and glass color
            t = (self.temp - 400) / (1973 - 400)
            sand = self._original_sand_color
            glass = pygame.Color("#949677AA")
            r = int(sand.r + (glass.r - sand.r) * t)
            g = int(sand.g + (glass.g - sand.g) * t)
            b = int(sand.b + (glass.b - sand.b) * t)
            a = int(sand.a + (glass.a - sand.a) * t)

            return pygame.Color(r, g, b, a)

    # @profile(stdout=False, filename='Sand-update.prof')
    def update(self, game_map, pos):
        if self.temp > 1973:  # 1700 *C, 3092 *F
            if not self._is_glass:
                self._is_glass = True
                GameSound('convert.Sand_to_glass').play()
            _fall_liquid(self, game_map, *pos)

        elif not self._is_glass:
            _fall_sand(self, game_map, *pos)


class Lubricant(BaseMaterial, display_name='Lubricant'):
    color = None
    heat_capacity = 0.7  # liquids store heat well
    thermal_conductivity = 0.3  # moderate transfer
    tags = MaterialTags.LIQUID

    def __init__(self, game_map, pos):
        self.color = pygame.Color(0, random.randint(0x95, 0xBB), 0x99)

    def update(self, game_map, pos):
        _fall_liquid(self, game_map, *pos)


class UnbreakableWall(BaseMaterial, display_name='Unbreakable Wall'):
    color = pygame.Color(0xFF, 0xFF, 0xFF)
    heat_capacity = 0  # does not change its temp
    thermal_conductivity = 0  # does not transfer heat
    tags = MaterialTags.SOLID

    def update(self, game_map, pos):
        # Unbreakable wall does not change
        pass


class Lava(BaseMaterial, display_name='Lava'):
    heat_capacity = 0.2  # hot, but doesn't store much
    thermal_conductivity = 0.6  # transfers heat well
    temp = 1200  # 927 *C, 1700 *F

    def __init__(self, game_map, pos):
        GameSound('material.Lava').play()

    def update(self, game_map, pos):
        if self.temp > 400:  # 127 *C, 260 *F
            _fall_liquid(self, game_map, *pos)

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

    def __new__(cls, game_map, pos):
        dot = game_map[pos]
        if dot is not None:
            dot.temp += 100
        return dot


class Minus100K(BaseMaterial, display_name='-100 K'):
    color = None
    heat_capacity = None
    thermal_conductivity = None
    tags = MaterialTags.NULL

    def __new__(cls, game_map, pos):
        dot = game_map[pos]
        if dot is not None:
            dot.temp -= 100
        return dot


class BlackHole(BaseMaterial, display_name='Black Hole'):
    color = pygame.Color("#1F1F1F")
    heat_capacity = 0
    thermal_conductivity = 0
    tags = MaterialTags.SOLID

    def update(self, game_map, pos):
        for rx, ry, _ in _von_neumann_hood(game_map, *pos, MaterialTags.MOVABLE):
            game_map[rx, ry] = Space(game_map, (rx, ry))


class Propane(BaseMaterial, display_name='Propane'):
    color = pygame.Color("#385DA345")
    heat_capacity = 0.3  # factors like Space got
    thermal_conductivity = 0.01
    tags = MaterialTags.GAS

    def update(self, game_map, pos):
        _fall_gas(self, game_map, *pos)
