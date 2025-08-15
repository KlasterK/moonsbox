import abc
import random
from enum import Enum, Flag, auto
from typing import TYPE_CHECKING, Any

import numpy as np
import pygame

from .config import DEFAULT_TEMP
from .util import blend, fast_randint, fast_random
from .soundengine import play_sound

if TYPE_CHECKING:
    from .gamemap import GameMap


available_materials = {}


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

    WET = auto()
    ELECTRIC = auto()


class PhysicalEntity(Enum):
    NULL = 0
    SAND = auto()
    LIQUID = auto()
    LIGHT_GAS = auto()
    HEAVY_GAS = auto()


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
    physical_entity = PhysicalEntity.NULL

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
            self.physical_entity = PhysicalEntity.LIQUID
            self.tags = MaterialTags.LIQUID

        elif not self._is_glass:
            self.physical_entity = PhysicalEntity.SAND
            self.tags = MaterialTags.BULK

        else:
            self.physical_entity = PhysicalEntity.NULL
            self.tags = MaterialTags.SOLID


class _CommonWater(BaseMaterial):
    heat_capacity = 0.7  # liquids store heat well
    thermal_conductivity = 0.3  # moderate transfer

    def __init__(self, game_map, x, y, converting=False):
        self._converting = converting
        super().__init__(game_map, x, y)  # calls __post_init__

    def _convert_to(self, type_: type['_CommonWater'], x: int, y: int, sound: str) -> None:
        new_dot = type_(self.map, x, y, converting=True)
        new_dot.temp = self.temp
        self.map[x, y] = new_dot
        play_sound(sound)


class Water(_CommonWater, display_name='Water'):
    tags = MaterialTags.LIQUID
    color = None
    physical_entity = PhysicalEntity.LIQUID

    def __post_init__(self, x, y):
        self.color = pygame.Color(0, random.randint(0x95, 0xBB), 0x99)
        if not self._converting:
            play_sound('material.Water')

    def update(self, x, y):
        if self.temp < 270:
            self._convert_to(Ice, x, y, 'convert.Water_freezes')
        elif self.temp > 375:
            self._convert_to(Steam, x, y, 'convert.Water_evaporates')


class Ice(_CommonWater, display_name='Ice'):
    temp = 220
    color = pygame.Color("#66C8E0B7")
    tags = MaterialTags.SOLID

    def __post_init__(self, x, y):
        if not self._converting:
            play_sound('material.Ice')

    def update(self, x, y):
        if self.temp > 275:
            self._convert_to(Water, x, y, 'convert.Ice_melts')


class Steam(Water, display_name='Steam'):
    temp = 420
    color = pygame.Color("#28BBC53D")
    tags = MaterialTags.GAS
    physical_entity = PhysicalEntity.LIGHT_GAS

    def __post_init__(self, x, y):
        if not self._converting:
            play_sound('material.Steam')

    def update(self, x, y):
        if self.temp < 370:
            self._convert_to(Water, x, y, 'convert.Steam_condensates')
        else:
            self.physical_entity = PhysicalEntity.LIGHT_GAS


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
            self.physical_entity = PhysicalEntity.LIQUID
            self.tags = MaterialTags.LIQUID
        else:
            self.physical_entity = PhysicalEntity.NULL
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
        elif fast_randint(1, 6) == 6:
            for rx, ry, _ in _von_neumann_hood(self.map, x, y, MaterialTags.SPACE):
                self.map[rx, ry] = self._generate_type(self.map, rx, ry)
        elif fast_randint(1, 30) == 16:  # exchange tap's generate type to other taps
            for _, _, dot in _moore_hood(self.map, x, y, MaterialTags.SOLID):
                if hasattr(dot, '_generate_type'):  # we could create a separate tag for taps
                    dot._generate_type = self._generate_type


class Propane(BaseMaterial, display_name='Propane'):
    color = pygame.Color("#385DA345")
    heat_capacity = 0.3  # factors that Space used to have
    thermal_conductivity = 0.5
    tags = MaterialTags.GAS
    physical_entity = PhysicalEntity.LIGHT_GAS

    def update(self, x, y):
        if self.temp > 700:  # ~500 *C
            self.map[x, y] = Fire(self.map, x, y)
            self.map[x, y].temp = 2800
            for rx, ry, _ in _moore_hood(self.map, x, y, MaterialTags.GAS):
                self.map[rx, ry] = Fire(self.map, rx, ry)
                self.map[rx, ry].temp = 2800

        elif self.tags & MaterialTags.SOLID:
            if self.temp > 85:
                self.physical_entity = PhysicalEntity.LIQUID
                self.tags = MaterialTags.LIQUID
                self.color = pygame.Color("#5376B885")

        elif self.tags & MaterialTags.LIQUID:
            if self.temp < 80:
                self.physical_entity = PhysicalEntity.NULL
                self.tags = MaterialTags.SOLID
                self.color = pygame.Color("#6D8EC9B8")
            elif self.temp > 235:
                self.physical_entity = PhysicalEntity.LIGHT_GAS
                self.tags = MaterialTags.GAS
                self.color = pygame.Color("#385DA345")

        elif self.tags & MaterialTags.GAS:
            if self.temp < 230:
                self.physical_entity = PhysicalEntity.LIQUID
                self.tags = MaterialTags.LIQUID
                self.color = pygame.Color("#5376B885")


class Fire(BaseMaterial, display_name='Fire'):
    heat_capacity = 1  # it must be 1 so fire cannot lose its heat
    thermal_conductivity = 1  # very high
    tags = MaterialTags.GAS
    temp = 1000  # about 800 *C, temp of wood burning
    physical_entity = PhysicalEntity.LIGHT_GAS

    def __post_init__(self, x, y):
        self._time_to_live = fast_randint(0, 20)
        play_sound('material.Fire')

    def update(self, x, y):
        if self._time_to_live <= 0:
            self.map[x, y] = Space(self.map, x, y)
            return

        self._time_to_live -= 1

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
            self.physical_entity = PhysicalEntity.LIQUID
        else:
            self.physical_entity = PhysicalEntity.NULL


class Absorbent(BaseMaterial, display_name='Absorbent'):
    heat_capacity = 0.2
    thermal_conductivity = 0.9
    color = pygame.Color("#eeeee3")
    tags = MaterialTags.FLOAT
    physical_entity = PhysicalEntity.SAND

    def __post_init__(self, x, y):
        grayscale = fast_randint(0xDD, 0xFF)
        yellowness = fast_randint(0x11, 0x33)
        self.color = pygame.Color(grayscale, grayscale, grayscale - yellowness)
        self._time_to_live = fast_randint(0, 200)

    def update(self, x, y):
        for rx, ry, _ in _moore_hood(self.map, x, y, MaterialTags.LIQUID):
            self.map[rx, ry] = Space(self.map, rx, ry)
            self._time_to_live -= 50

        if self._time_to_live < 0:
            self.map[x, y] = Space(self.map, x, y)
        else:
            self._time_to_live -= 1


class Aerogel(BaseMaterial, display_name='Aerogel'):
    heat_capacity = 0.99
    thermal_conductivity = 0.01
    tags = MaterialTags.FLOAT
    color = None

    def __post_init__(self, x, y):
        grayscale = fast_randint(0xAA, 0xBB)
        self.color = pygame.Color(grayscale, grayscale, grayscale, 0x25)


class DryIce(BaseMaterial, display_name='Dry Ice'):
    heat_capacity = 0.95
    thermal_conductivity = 1
    tags = MaterialTags.BULK
    color = None
    temp = 175

    def __post_init__(self, x, y):
        self._orig_color = blend(pygame.Color("#dbe2ee"), pygame.Color("#c2d9df"), fast_random())

    def update(self, x, y):
        if self.temp > 250:
            self.map[x, y] = Space(self.map, x, y)
            self.physical_entity = PhysicalEntity.NULL
        elif self.temp > 195:
            self.tags = MaterialTags.GAS
            self.physical_entity = PhysicalEntity.HEAVY_GAS
        else:
            self.tags = MaterialTags.BULK
            self.physical_entity = PhysicalEntity.SAND

    @property
    def color(self):
        color = pygame.Color(self._orig_color)
        factor = 1 - (self.temp - 175) / (250 - 175)
        color.a = min(255, max(0, int(factor * 255)))
        return color


class Flour(BaseMaterial, display_name='Flour'):
    heat_capacity = 0.12
    thermal_conductivity = 0.12
    tags = MaterialTags.BULK
    color = None

    def __post_init__(self, x, y):
        grayscale = random.randint(0xCC, 0xFF)
        self._orig_color = pygame.Color((grayscale,) * 3)

    def update(self, x, y):
        if self.temp > 500:  # 300 *C
            self.map[x, y] = Fire(self.map, x, y)

        if not self.tags & MaterialTags.WET:
            for rx, ry, _ in _moore_hood(self.map, x, y, MaterialTags.WET | MaterialTags.LIQUID):
                self.map[rx, ry] = Space(self.map, rx, ry)
                self.tags |= MaterialTags.WET

            self._fall_ash(x, y)
        else:
            for rx, ry, dot in _von_neumann_hood(self.map, x, y, MaterialTags.BULK):
                if not dot.tags & MaterialTags.WET:
                    dot.tags |= MaterialTags.WET
                    self.tags &= ~MaterialTags.WET

    @property
    def color(self):
        if self.tags & MaterialTags.WET:
            return pygame.Color("#D3CFBC")
        return self._orig_color


# ======== Electronics ========


class BaseElec(BaseMaterial):
    potential = 0
    current = 0

    def __post_init__(self, x, y):
        total_v = 0
        weights = 0

        for _, _, nb_dot in _von_neumann_hood(self.map, x, y, MaterialTags.ELECTRIC):
            r = self.resistance + nb_dot.resistance
            if r > 0:
                weight = 1 / r
                total_v += nb_dot.voltage * weight
                weights += weight

        if weights > 0:
            self.voltage = total_v / weights


class Copper(BaseElec, display_name='Copper'):
    heat_capacity = 0.7
    thermal_conductivity = 0.87
    tags = MaterialTags.SOLID | MaterialTags.ELECTRIC
    color = pygame.Color("#D47216")

    resistance = 0.01

    def update(self, x, y):
        self._elec_update(x, y)


class LightBulb(BaseElec, display_name='Light Bulb'):
    '''1 watt incandescent lamp.'''

    heat_capacity = 0.3
    thermal_conductivity = 0.6
    tags = MaterialTags.SOLID | MaterialTags.ELECTRIC
    color = None

    resistance = 1

    def update(self, x, y):
        self._elec_update(x, y)

    @property
    def color(self):
        power = abs(self.voltage**2) / self.resistance
        return blend(pygame.Color("#4E4E4E84"), pygame.Color("#ffbb00"), power)


class PotentialPlus1V(BaseElec, display_name='Potential +1 V'):
    heat_capacity = 0.5
    thermal_conductivity = 0.3
    tags = MaterialTags.SOLID | MaterialTags.ELECTRIC
    color = pygame.Color("#542c5e")

    def update(self, x, y):
        self.voltage = max(1, self.voltage)
        self._elec_update(x, y)


class Potential0V(BaseElec, display_name='Potential 0 V'):
    heat_capacity = 0.5
    thermal_conductivity = 0.5
    tags = MaterialTags.SOLID | MaterialTags.ELECTRIC
    color = pygame.Color("#4b2455")
    voltage = 0

    def update(self, x, y):
        self.voltage = max(0, self.voltage)
        self._elec_update(x, y)
