import abc
import random
from enum import Flag, auto
from typing import TYPE_CHECKING, Any

import numpy as np
import pygame

from .config import DEFAULT_TEMP
from .util import blend
from .soundengine import play_sound

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


def _fast_random() -> float:
    global _random_index
    if _random_index >= _pre_generated_random.shape[0]:
        _random_index = 0

    ret = _pre_generated_random[_random_index]
    _random_index += 1
    return ret


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
    FLOAT = auto()
    SPARSENESS = GAS | SPACE
    FLOWABLE = SPARSENESS | LIQUID
    MOVABLE = BULK | LIQUID | GAS | FLOAT


class BaseMaterial(abc.ABC):
    '''Abstract base class for materials.'''

    def __init__(self, game_map, x, y):
        self.map: 'GameMap' = game_map
        self.__post_init__(x, y)

    def __post_init__(self, x: int, y: int):
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

    def update(self, x: int, y: int) -> None:
        '''Updates physical processes of a dot.'''

    def _diffuse(self, x, y, tags, chance=0.01):
        if _fast_random() > chance:
            return

        nb_pos = ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))[_fast_randint(0, 3)]
        nb_dot = self.map[nb_pos]

        if nb_dot is None:
            return

        if nb_dot.tags & tags:
            self.map[nb_pos] = self
            self.map[x, y] = nb_dot

    def _fall_light_gas(self, x, y):
        for remote_pos in (
            (x, y + 1),
            _fast_choice2((x - 1, y), (x + 1, y)),
            _fast_choice2((x - 1, y + 1), (x + 1, y + 1)),
        ):
            remote_dot = self.map[remote_pos]
            if remote_dot is not None and remote_dot.tags & MaterialTags.SPACE:
                self.map[remote_pos] = self
                self.map[x, y] = remote_dot
                return

        self._diffuse(x, y, MaterialTags.GAS)

    def _fall_heavy_gas(self, x, y):
        neighbours = [
            (x, y + 1),
            (x + 1, y),
            (x - 1, y),
            (x, y - 1),
            (x + 1, y + 1),
            (x + 1, y - 1),
            (x - 1, y + 1),
            (x - 1, y - 1),
        ]
        np.random.shuffle(neighbours)

        for remote_pos in neighbours:
            remote_dot = self.map[remote_pos]
            if remote_dot is not None and remote_dot.tags & MaterialTags.SPACE:
                self.map[remote_pos] = self
                self.map[x, y] = remote_dot
                break

        self._diffuse(x, y, MaterialTags.GAS)

    def _fall_liquid(self, x, y):
        remote_dot = self.map[x, y + 1]
        if remote_dot is not None and remote_dot.tags & MaterialTags.BULK:
            self.map[x, y + 1] = self
            self.map[x, y] = remote_dot
            return

        for remote_pos in (
            (x, y - 1),
            _fast_choice2((x - 1, y), (x + 1, y)),
            _fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
        ):
            remote_dot = self.map[remote_pos]
            if remote_dot is not None and remote_dot.tags & MaterialTags.SPARSENESS:
                self.map[remote_pos] = self
                self.map[x, y] = remote_dot
                return

        self._diffuse(x, y, MaterialTags.LIQUID)

    def _fall_sand(self, x, y):
        for nb_pos in (x, y - 1), _fast_choice2((x - 1, y - 1), (x + 1, y - 1)):
            nb_dot = self.map[nb_pos]

            if nb_dot is None:
                continue

            if nb_dot.tags & MaterialTags.FLOWABLE:
                self.map[nb_pos] = self
                self.map[x, y] = nb_dot
                break

    def _fall_ash(self, x, y):
        down = (x, y - 1)
        side = _fast_choice2((x - 1, y - 1), (x + 1, y - 1))

        for remote_pos in (side,) + _fast_choice2((), (down,)):
            remote_dot = self.map[remote_pos]

            if remote_dot is not None and remote_dot.tags & MaterialTags.SPARSENESS:
                self.map[remote_pos] = self
                self.map[x, y] = remote_dot
                break


class Space(BaseMaterial, display_name='Space'):  # air
    color = pygame.Color(0, 0, 0, 0)
    heat_capacity = 0.3  # moderate, air easily changes temp
    thermal_conductivity = 1  # max value
    tags = MaterialTags.SPACE


class Sand(BaseMaterial, display_name='Sand'):
    heat_capacity = 0.3  # moderate, sand stores some heat
    thermal_conductivity = 0.1  # sand transfers heat slowly
    tags = MaterialTags.BULK

    def __post_init__(self, x, y):
        self._is_glass = False
        self._original_sand_color = pygame.Color(0xFF, random.randint(0x99, 0xFF), 0)
        play_sound('material.Sand')

    @property
    def color(self):
        t = (self.temp - 400) / (1973 - 400)
        if self._is_glass:
            return blend(pygame.Color("#96947755"), pygame.Color("#FF880085"), t)
        else:
            return blend(self._original_sand_color, pygame.Color("#FF6600AA"), t)

    def update(self, x, y):
        if self.temp > 1973:  # 1700 *C, 3092 *F
            if not self._is_glass:
                self._is_glass = True
                play_sound('convert.Sand_to_glass')
            self._fall_liquid(x, y)
            self.tags = MaterialTags.LIQUID

        elif not self._is_glass:
            self._fall_sand(x, y)
            self.tags = MaterialTags.BULK

        else:
            self.tags = MaterialTags.SOLID


class Water(BaseMaterial, display_name='Water'):
    heat_capacity = 0.7  # liquids store heat well
    thermal_conductivity = 0.3  # moderate transfer
    tags = MaterialTags.LIQUID
    color = None

    def __post_init__(self, x, y):
        self._liquid_color = pygame.Color(0, random.randint(0x95, 0xBB), 0x99)
        if self.color is None:
            self.color = self._liquid_color

        if self.tags & MaterialTags.SOLID:
            play_sound('material.Ice')
        elif self.tags & MaterialTags.LIQUID:
            play_sound('material.Water')
        elif self.tags & MaterialTags.GAS:
            play_sound('material.Steam')

    def update(self, x, y):
        if self.tags & MaterialTags.SOLID:
            if self.temp > 275:
                self.tags = MaterialTags.LIQUID
                self.color = self._liquid_color
                play_sound('convert.Ice_melts')

        elif self.tags & MaterialTags.LIQUID:
            if self.temp < 270:
                self.tags = MaterialTags.SOLID
                self.color = pygame.Color("#66C8E0B7")
                play_sound('convert.Water_freezes')
            elif self.temp > 375:
                self.tags = MaterialTags.GAS
                self.color = pygame.Color("#28BBC53D")
                play_sound('convert.Water_evaporates')
            self._fall_liquid(x, y)

        elif self.tags & MaterialTags.GAS:
            if self.temp < 370:
                self.tags = MaterialTags.LIQUID
                self.color = self._liquid_color
                play_sound('convert.Steam_condensates')
            self._fall_light_gas(x, y)


class Ice(Water, display_name='Ice'):
    temp = 220
    color = pygame.Color("#66C8E0B7")
    tags = MaterialTags.SOLID


class Steam(Water, display_name='Steam'):
    temp = 420
    color = pygame.Color("#28BBC53D")
    tags = MaterialTags.GAS


class UnbreakableWall(BaseMaterial, display_name='Unbreakable Wall'):
    color = pygame.Color(0xFF, 0xFF, 0xFF)
    heat_capacity = 0.6  # does not change its temp
    thermal_conductivity = 0.4  # does not transfer heat
    tags = MaterialTags.SOLID


class Lava(BaseMaterial, display_name='Lava'):
    heat_capacity = 0.8  # hot, but doesn't store much
    thermal_conductivity = 0.5  # transfers heat well
    temp = 1200  # 927 *C, 1700 *F
    tags = MaterialTags.LIQUID

    def __post_init__(self, x, y):
        play_sound('material.Lava')

    def update(self, x, y):
        if self.temp > 400:  # 127 *C, 260 *F
            self._fall_liquid(x, y)
            self.tags = MaterialTags.LIQUID
        else:
            self.tags = MaterialTags.SOLID

    @property
    def color(self):
        factor = (self.temp - 400) / (1200 - 400)
        if factor > 0.5:
            return blend(pygame.Color("#ff0000"), pygame.Color("#ffff00"), (factor - 0.5) * 2)
        else:
            return blend(pygame.Color("#440000"), pygame.Color("#ff0000"), factor * 2)


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
        if dot.temp < 0:
            dot.temp = 0
        return dot


class BlackHole(BaseMaterial, display_name='Black Hole'):
    color = pygame.Color("#1F1F1F")
    heat_capacity = 0
    thermal_conductivity = 0
    tags = MaterialTags.SOLID

    def update(self, x, y):
        for rx, ry, _ in _von_neumann_hood(self.map, x, y, MaterialTags.MOVABLE):
            self.map[rx, ry] = Space(self.map, rx, ry)


class Tap(BaseMaterial, display_name='Tap'):
    color = pygame.Color("#67a046")
    heat_capacity = 0.2  # factors that Lava used to have
    thermal_conductivity = 0.6
    tags = MaterialTags.SOLID

    def __post_init__(self, x, y):
        self._generate_type = None

    def update(self, x, y):
        if self._generate_type is None:
            for rx, ry, dot in _von_neumann_hood(self.map, x, y, MaterialTags.MOVABLE):
                self._generate_type = type(dot)
                break
        elif _fast_randint(1, 6) == 6:
            for rx, ry, _ in _von_neumann_hood(self.map, x, y, MaterialTags.SPACE):
                self.map[rx, ry] = self._generate_type(self.map, rx, ry)
        elif _fast_randint(1, 30) == 16:  # exchange tap's generate type to other taps
            for _, _, dot in _moore_hood(self.map, x, y, MaterialTags.SOLID):
                if hasattr(dot, '_generate_type'):  # we could create a separate tag for taps
                    dot._generate_type = self._generate_type


class Propane(BaseMaterial, display_name='Propane'):
    color = pygame.Color("#385DA345")
    heat_capacity = 0.3  # factors that Space used to have
    thermal_conductivity = 0.5
    tags = MaterialTags.GAS

    def update(self, x, y):
        if self.temp > 700:  # ~500 *C
            self.map[x, y] = Fire(self.map, x, y)
            self.map[x, y].temp = 2800
            for rx, ry, _ in _moore_hood(self.map, x, y, MaterialTags.GAS):
                self.map[rx, ry] = Fire(self.map, rx, ry)
                self.map[rx, ry].temp = 2800

        elif self.tags & MaterialTags.SOLID:
            if self.temp > 85:
                self.tags = MaterialTags.LIQUID
                self.color = pygame.Color("#5376B885")

        elif self.tags & MaterialTags.LIQUID:
            if self.temp < 80:
                self.tags = MaterialTags.SOLID
                self.color = pygame.Color("#6D8EC9B8")
            elif self.temp > 235:
                self.tags = MaterialTags.GAS
                self.color = pygame.Color("#385DA345")
            self._fall_liquid(x, y)

        elif self.tags & MaterialTags.GAS:
            if self.temp < 230:
                self.tags = MaterialTags.LIQUID
                self.color = pygame.Color("#5376B885")
            self._fall_light_gas(x, y)


class Fire(BaseMaterial, display_name='Fire'):
    heat_capacity = 1  # it must be 1 so fire cannot lose its heat
    thermal_conductivity = 1  # very high
    tags = MaterialTags.GAS
    temp = 1000  # about 800 *C, temp of wood burning

    def __post_init__(self, x, y):
        self._time_to_live = _fast_randint(0, 20)
        play_sound('material.Fire')

    def update(self, x, y):
        if self._time_to_live <= 0:
            self.map[x, y] = Space(self.map, x, y)
            return

        self._time_to_live -= 1
        self._fall_light_gas(x, y)

    @property
    def color(self):
        factor = self._time_to_live / 20
        return blend(pygame.Color("#ff000044"), pygame.Color("#FFff00"), factor)


class PureGlass(BaseMaterial, display_name='Glass'):
    heat_capacity = 0.5
    thermal_conductivity = 0.05

    @property
    def color(self):
        t = (self.temp - 400) / (1773 - 400)
        return blend(pygame.Color("#53D49820"), pygame.Color("#FF880085"), t)

    @property
    def tags(self):
        if self.temp > 1773:
            return MaterialTags.LIQUID
        return MaterialTags.SOLID

    def update(self, x, y):
        if self.temp > 1773:  # 1500 *C
            self._fall_liquid(x, y)


class Absorbent(BaseMaterial, display_name='Absorbent'):
    heat_capacity = 0.2
    thermal_conductivity = 0.9
    color = pygame.Color("#eeeee3")
    tags = MaterialTags.FLOAT

    def __post_init__(self, x, y):
        grayscale = _fast_randint(0xDD, 0xFF)
        yellowness = _fast_randint(0x11, 0x33)
        self.color = pygame.Color(grayscale, grayscale, grayscale - yellowness)
        self._time_to_live = _fast_randint(0, 200)

    def update(self, x, y):
        for rx, ry, _ in _moore_hood(self.map, x, y, MaterialTags.LIQUID):
            self.map[rx, ry] = Space(self.map, rx, ry)
            self._time_to_live -= 50

        if self._time_to_live < 0:
            self.map[x, y] = Space(self.map, x, y)
        else:
            self._time_to_live -= 1
            self._fall_sand(x, y)


class Aerogel(BaseMaterial, display_name='Aerogel'):
    heat_capacity = 0.99
    thermal_conductivity = 0.01
    tags = MaterialTags.FLOAT
    color = None

    def __post_init__(self, x, y):
        grayscale = _fast_randint(0xAA, 0xBB)
        self.color = pygame.Color(grayscale, grayscale, grayscale, 0x25)


class DryIce(BaseMaterial, display_name='Dry Ice'):
    heat_capacity = 0.95
    thermal_conductivity = 1
    tags = MaterialTags.BULK
    color = None
    temp = 175

    def __post_init__(self, x, y):
        self._orig_color = blend(pygame.Color("#dbe2ee"), pygame.Color("#c2d9df"), _fast_random())

    def update(self, x, y):
        if self.temp > 250:
            self.map[x, y] = Space(self.map, x, y)
        elif self.temp > 195:
            self.tags = MaterialTags.GAS
            self._fall_heavy_gas(x, y)
        else:
            self.tags = MaterialTags.BULK
            self._fall_sand(x, y)

    @property
    def color(self):
        color = pygame.Color(self._orig_color)
        factor = 1 - (self.temp - 175) / (250 - 175)
        color.a = min(255, max(0, int(factor * 255)))
        return color
