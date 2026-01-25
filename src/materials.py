from enum import IntEnum, IntFlag, auto
from .libopt import ls_materials


available_materials = ls_materials()


class MaterialTags(IntFlag):
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


class PhysicalEntity(IntEnum):
    NULL = 0
    SAND = auto()
    LIQUID = auto()
    LIGHT_GAS = auto()
    HEAVY_GAS = auto()
