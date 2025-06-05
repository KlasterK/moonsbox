import abc
import random
from typing import TYPE_CHECKING, Any

import pygame

from . import DEFAULT_TEMP, GameSound

if TYPE_CHECKING:
    from .gamemap import GameMap


available_materials = {}

_pre_generated_choice2 = random.choices((0, 1), k=100)
_choice2_index = 0


def _fast_choice2(a: Any, b: Any) -> Any:
    global _choice2_index
    _choice2_index += 1
    return (a, b)[_pre_generated_choice2[_choice2_index % 100]]
    return a


_last_material_flag_index = -1


def _unique_material_flag() -> int:
    # returns a unique bit flag for a material
    global _last_material_flag_index
    _last_material_flag_index += 1
    return 1 << _last_material_flag_index


def _fast_isinstance(obj: Any, cls: type['BaseMaterial']) -> bool:
    try:
        return obj.material_flags & cls.MATERIAL_FLAG
    except AttributeError:
        return False


class BaseMaterial(abc.ABC):
    '''Abstract base class for materials.'''

    def __new__(cls, game_map, pos: tuple[int, int] | None):
        return super().__new__(cls)

    def __init__(self, game_map, pos: tuple[int, int] | None):
        pass

    def __init_subclass__(cls, display_name: str = ''):
        super().__init_subclass__()

        cls.MATERIAL_FLAG = cls.material_flags = _unique_material_flag()
        for base in cls.__bases__:
            cls.material_flags |= getattr(base, 'material_flags', 0)

        if display_name:
            available_materials[display_name] = cls

    @property
    @abc.abstractmethod
    def color(self) -> pygame.Color:
        '''Color of a dot.'''

    temp = DEFAULT_TEMP
    material_flags = 0

    @property
    @abc.abstractmethod
    def heat_capacity(self) -> int:
        '''Heat capacity factor of a dot.'''

    @property
    @abc.abstractmethod
    def thermal_conductivity(self) -> int:
        '''Thermal conductivity factor of a dot.'''

    def update(self, game_map: 'GameMap', pos: tuple[int, int]) -> None:
        '''Updates physical processes of a dot.'''


class Space(BaseMaterial, display_name='Space'):  # air
    color = pygame.Color(0, 0, 0, 0)
    heat_capacity = 0.3  # moderate, air easily changes temp
    thermal_conductivity = 1  # low but not 0, air transfers heat slowly


class Sand(BaseMaterial, display_name='Sand'):
    heat_capacity = 0.3  # moderate, sand stores some heat
    thermal_conductivity = 0.1  # sand transfers heat slowly

    def __init__(self, game_map, pos):
        self.temp = DEFAULT_TEMP
        self._is_glass = False
        self._original_sand_color = pygame.Color(0xFF, random.randint(0x99, 0xFF), 0)
        GameSound('material_Sand').play()

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
                GameSound('convert_Sand_to_glass').play()

            x, y = pos
            for remote_pos in (
                (x, y - 1),
                _fast_choice2((x - 1, y), (x + 1, y)),
                _fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
            ):
                remote_dot = game_map[remote_pos]
                if _fast_isinstance(remote_dot, Space):
                    game_map[remote_pos] = self
                    game_map[x, y] = remote_dot
                    break

        elif not self._is_glass:
            x, y = pos
            for remote_pos in (x, y - 1), _fast_choice2((x - 1, y - 1), (x + 1, y - 1)):
                remote_dot = game_map[remote_pos]

                if _fast_isinstance(remote_dot, Space):
                    game_map[remote_pos] = self
                    game_map[x, y] = remote_dot
                    break

        # if y % 2:
        #     dot = game_map[x, y - 1]
        #     if _fast_isinstance(dot, Space):
        #         game_map[x, y - 1] = self
        #         game_map[x, y] = dot
        # else:
        #     dot = game_map[x, y + 1]
        #     if _fast_isinstance(dot, Space):
        #         game_map[x, y + 1] = self
        #         game_map[x, y] = dot

        # self._death_counter += 1
        # if self._death_counter > 1000:
        #     game_map[pos] = Space()


class InertLiquid(BaseMaterial, display_name='Inert Liquid'):
    color = None
    heat_capacity = 0.7  # liquids store heat well
    thermal_conductivity = 0.3  # moderate transfer

    def __init__(self, game_map, pos):
        self.color = pygame.Color(0, random.randint(0x95, 0xBB), 0x99)

    def update(self, game_map, pos):
        x, y = pos

        remote_dot = game_map[x, y + 1]
        if _fast_isinstance(remote_dot, Sand):
            game_map[x, y + 1] = self
            game_map[x, y] = remote_dot
            return

        for remote_pos in (
            (x, y - 1),
            _fast_choice2((x - 1, y), (x + 1, y)),
            _fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
        ):
            remote_dot = game_map[remote_pos]
            if _fast_isinstance(remote_dot, Space):
                game_map[remote_pos] = self
                game_map[x, y] = remote_dot
                break


class UnbreakableWall(BaseMaterial, display_name='Unbreakable Wall'):
    color = pygame.Color(0xFF, 0xFF, 0xFF)
    heat_capacity = 0  # does not change its temp
    thermal_conductivity = 0  # does not transfer heat

    def update(self, game_map, pos):
        # Unbreakable wall does not change
        pass


class Lava(BaseMaterial, display_name='Lava'):
    heat_capacity = 0.2  # hot, but doesn't store much
    thermal_conductivity = 0.6  # transfers heat well
    temp = 1200  # 927 *C, 1700 *F

    def __init__(self, game_map, pos):
        GameSound('material_Lava').play()

    def update(self, game_map, pos):
        if self.temp > 400:  # 127 *C, 260 *F
            x, y = pos

            for remote_pos in (
                (x, y - 1),
                _fast_choice2((x - 1, y), (x + 1, y)),
                _fast_choice2((x - 1, y - 1), (x + 1, y - 1)),
            ):
                remote_dot = game_map[remote_pos]
                if _fast_isinstance(remote_dot, Space):
                    game_map[remote_pos] = self
                    game_map[x, y] = remote_dot
                    break

    @property
    def color(self):
        mid_temp = self.temp - 1200
        red = mid_temp / 5 + 0xFF
        green = mid_temp / 8 + 0x10
        return pygame.Color(max(0x44, min(255, red)), max(0, min(255, green)), 0)


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

    def __new__(cls, game_map, pos):
        dot = game_map[pos]
        if dot is not None:
            dot.temp -= 100
        return dot
