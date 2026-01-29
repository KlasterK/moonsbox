import abc
from pathlib import Path
from typing import TYPE_CHECKING

import pygame

from .config import (
    DEFAULT_DRAWING_WIDTH,
    DELTA_DRAWING_WIDTH,
    DRAWING_IS_CIRCULAR,
    DRAWING_IS_DESTRUCTIVE,
    ZOOM_FACTOR,
    PALETTE_MOUSE_SELECTION_REQUIRES_DBL_CLICK,
    ASSETS_ROOT,
    MUSIC_VOLUME,
)
from .libopt import GameMap, SimulationManager, Renderer
from .materialpalette import MaterialPalette
from .materials import MaterialTags, available_materials
from .soundengine import play_sound

if TYPE_CHECKING:
    from .gameapp import Camera, GameApp


class StopHandling(Exception):
    '''Exception to prevent the current event from handling by other event handlers.

    If a handler raises it, make sure that the handler is above the others.
    '''


class BaseEventHandler(abc.ABC):
    @abc.abstractmethod
    def process_event(self, e: pygame.Event):
        pass

    _common_user_event_type = pygame.event.custom_type()


class GameAppEventHandler(BaseEventHandler):
    def __init__(self, app: 'GameApp'):
        self._app = app

    def process_event(self, e):
        if e.type == pygame.QUIT:
            self._app.stop()

        elif e.type == pygame.KEYDOWN:
            if e.key == pygame.K_ESCAPE:
                self._app.stop()


class SimulationEventHandler(BaseEventHandler):
    def __init__(self, game_map: GameMap, simulation_manager: SimulationManager):
        self._map = game_map
        self._sim = simulation_manager

    def process_event(self, e):
        if e.type == pygame.KEYDOWN:
            if e.key == pygame.K_F5:
                old_pause_state = self._sim.is_paused()
                self._sim.set_paused(False)
                self._sim.tick()
                self._sim.set_paused(old_pause_state)

            elif e.key == pygame.K_F6:
                self._map.fill(available_materials['Space'])

            elif e.key == pygame.K_SPACE:
                self._sim.set_paused(not self._sim.is_paused())


class CameraEventHandler(BaseEventHandler):
    def __init__(self, camera: 'Camera'):
        self._camera = camera

    def process_event(self, e):
        if e.type == pygame.MOUSEMOTION:
            mods = pygame.key.get_mods()

            if mods & pygame.KMOD_SHIFT or e.buttons[2]:
                self._camera.move(-e.rel[0], -e.rel[1])

        elif e.type == pygame.MOUSEWHEEL:
            mods = pygame.key.get_mods()
            if mods & pygame.KMOD_ALT:
                return

            self._camera.zoom(-e.precise_y * ZOOM_FACTOR)


class DrawingEventHandler(BaseEventHandler):
    def __init__(self, camera: 'Camera', game_map: GameMap, palette: MaterialPalette):
        self._camera = camera
        self._map = game_map
        self._pal = palette
        self._previous_pos = None
        self._width = DEFAULT_DRAWING_WIDTH
        self._is_hold_drawing_event_sent = False

    def _material_factory(self, game_map, x, y):
        ready_dot = self._pal.selected_material(game_map, x, y)

        if ready_dot.tags & MaterialTags.SPACE:
            return ready_dot
        elif not DRAWING_IS_DESTRUCTIVE and not game_map[x, y].tags & MaterialTags.SPACE:
            return game_map[x, y]
        return ready_dot

    def process_event(self, e):
        if (
            e.type == pygame.KEYDOWN
            and e.key in (pygame.K_LCTRL, pygame.K_RCTRL)
            or e.type == pygame.MOUSEBUTTONDOWN
            and e.button == pygame.BUTTON_LEFT
        ):
            mouse_pos = pygame.mouse.get_pos()
            abs_pos = self._map.invy_pos(self._camera.convert_pos(mouse_pos))

            rect = pygame.Rect(0, 0, self._width, self._width)
            rect.center = abs_pos
            if DRAWING_IS_CIRCULAR:
                self._map.draw_ellipse(rect, self._material_factory)
            else:
                self._map.draw_rect(rect, self._material_factory)
            self._previous_pos = abs_pos

            pygame.event.post(
                pygame.event.Event(BaseEventHandler._common_user_event_type, purpose='hold-drawing')
            )

        elif e.type == pygame.MOUSEMOTION:
            if self._previous_pos is not None:
                mouse_pos = pygame.mouse.get_pos()
                abs_pos = self._map.invy_pos(self._camera.convert_pos(mouse_pos))

                self._map.draw_line(
                    self._previous_pos,
                    abs_pos,
                    self._width,
                    self._material_factory,
                    'none',
                )

                rect = pygame.Rect(0, 0, self._width, self._width)
                rect.center = abs_pos

                self._previous_pos = abs_pos

        elif (
            e.type == pygame.KEYUP
            and e.key in (pygame.K_LCTRL, pygame.K_RCTRL)
            or e.type == pygame.MOUSEBUTTONUP
            and e.button == pygame.BUTTON_LEFT
        ):
            self._previous_pos = None

        elif e.type == pygame.KEYDOWN and e.key == pygame.K_EQUALS:
            self._width += DELTA_DRAWING_WIDTH

        elif e.type == pygame.KEYDOWN and e.key == pygame.K_MINUS:
            self._width = max(1, self._width - DELTA_DRAWING_WIDTH)

        elif e.type == pygame.MOUSEWHEEL and pygame.key.get_mods() & pygame.KMOD_ALT:
            self._width = max(1, self._width + e.y)

        elif (
            e.type == BaseEventHandler._common_user_event_type
            and getattr(e, 'purpose', None) == 'hold-drawing'
        ):
            if self._previous_pos is not None:
                rect = pygame.Rect(0, 0, self._width, self._width)
                rect.center = self._previous_pos
                if DRAWING_IS_CIRCULAR:
                    self._map.draw_ellipse(rect, self._material_factory)
                else:
                    self._map.draw_rect(rect, self._material_factory)

                pygame.event.post(
                    pygame.event.Event(
                        BaseEventHandler._common_user_event_type, purpose='hold-drawing'
                    )
                )

        elif e.type == pygame.KEYDOWN and e.key == pygame.K_k:
            # Picker
            mouse_pos = pygame.mouse.get_pos()
            abs_pos = self._map.invy_pos(self._camera.convert_pos(mouse_pos))

            if not self._map.bounds(abs_pos):
                return

            material_type = type(self._map[abs_pos])
            self._pal.selected_material = material_type


class MaterialPaletteEventHandler(BaseEventHandler):
    def __init__(self, game_app: 'GameApp', palette: MaterialPalette):
        self._app = game_app
        self._pal = palette
        self._previous_material = None
        self._material_stack = []

    def process_event(self, e):
        if self._pal.is_visible():
            if e.type == pygame.KEYDOWN:
                if e.key in (pygame.K_LEFT, pygame.K_a):
                    x, y = self._pal.selection_slot
                    x = max(x - 1, 0)
                    self._pal.selection_slot = (x, y)
                    play_sound('palette.move_selection', 'ui', True)

                elif e.key in (pygame.K_RIGHT, pygame.K_d):
                    x, y = self._pal.selection_slot
                    width = self._pal.whole_grid_size[0]
                    x = min(x + 1, width - 1)
                    self._pal.selection_slot = (x, y)
                    play_sound('palette.move_selection', 'ui', True)

                elif e.key in (pygame.K_UP, pygame.K_w):
                    x, y = self._pal.selection_slot
                    y = max(y - 1, 0)
                    self._pal.selection_slot = (x, y)
                    play_sound('palette.move_selection', 'ui', True)

                elif e.key in (pygame.K_DOWN, pygame.K_s):
                    x, y = self._pal.selection_slot
                    height = self._pal.whole_grid_size[1]
                    y = min(y + 1, height - 1)
                    self._pal.selection_slot = (x, y)
                    play_sound('palette.move_selection', 'ui', True)

                elif e.key in (pygame.K_RETURN, pygame.K_SPACE, pygame.K_LCTRL, pygame.K_RCTRL):
                    self._pal.hide(True)
                    play_sound('palette.hide_confirmation', 'ui', True)

                elif e.key == pygame.K_ESCAPE:
                    self._pal.hide(False)
                    play_sound('palette.hide_no_confirmation', 'ui', True)

                raise StopHandling

            elif e.type == pygame.MOUSEBUTTONDOWN:
                if e.button == pygame.BUTTON_LEFT:
                    previous_slot_pos = self._pal.selection_slot

                    for slot_pos, rect in self._pal.grid_geometry.items():
                        if rect.collidepoint(e.pos):
                            self._pal.selection_slot = slot_pos

                            if (
                                PALETTE_MOUSE_SELECTION_REQUIRES_DBL_CLICK
                                and slot_pos != previous_slot_pos
                            ):
                                play_sound('palette.move_selection', 'ui', True)
                            else:
                                self._pal.hide(True)
                                play_sound('palette.hide_confirmation', 'ui', True)
                            break
                    else:
                        # If no slot was clicked, hide the palette
                        self._pal.hide(False)
                        play_sound('palette.hide_no_confirmation', 'ui', True)

                elif e.button == pygame.BUTTON_RIGHT:
                    self._pal.hide(False)
                    play_sound('palette.hide_no_confirmation', 'ui', True)

                raise StopHandling

            elif e.type == pygame.MOUSEWHEEL:
                height = self._pal.whole_grid_size[1]
                self._pal.current_row -= e.y

                if self._pal.current_row < 0:
                    self._pal.current_row = 0
                elif self._pal.current_row >= height:
                    self._pal.current_row = height - 1

                raise StopHandling

        else:
            if (
                e.type == pygame.KEYDOWN
                and e.key == pygame.K_TAB
                or e.type == pygame.MOUSEBUTTONDOWN
                and e.button == pygame.BUTTON_MIDDLE
            ):
                self._pal.show()
                play_sound('palette.show', 'ui', True)

            elif (
                e.type == pygame.KEYDOWN
                and e.key == pygame.K_u
                or e.type == pygame.MOUSEBUTTONDOWN
                and e.button == pygame.BUTTON_X2  # navigate forward
            ):
                self._material_stack.append(self._pal.selected_material)

            elif (
                e.type == pygame.KEYDOWN
                and e.key == pygame.K_i
                or e.type == pygame.MOUSEBUTTONDOWN
                and e.button == pygame.BUTTON_X1  # navigate back
            ):
                if not self._material_stack:
                    self._pal.selected_material = available_materials['Space']
                else:
                    self._pal.selected_material = self._material_stack.pop()


class RendererEventHandler(BaseEventHandler):
    def __init__(self, rnd: 'Renderer'):
        self._rnd = rnd

    def process_event(self, e):
        if e.type == pygame.KEYDOWN:
            if e.key == pygame.K_F10:
                self._rnd.take_screenshot()

            elif e.key == pygame.K_v:
                self._rnd.next_render_mask()


class MusicEventHandler(BaseEventHandler):
    def __init__(self, name: str):
        self._end_event_type = pygame.event.custom_type()
        pygame.mixer_music.set_endevent(self._end_event_type)
        pygame.mixer_music.set_volume(MUSIC_VOLUME)

        glob_path = Path(ASSETS_ROOT / 'sounds')
        self._tracks = sorted(glob_path.glob(f'{name}.*'))

        if not self._tracks:
            self._track_idx = -1
            return

        self._track_idx = 0

        pygame.mixer_music.load(self._tracks[0])
        pygame.mixer_music.play()

    def process_event(self, e):
        if e.type == self._end_event_type:
            self._track_idx += 1
            self._track_idx %= len(self._tracks)

            pygame.mixer_music.load(self._tracks[self._track_idx])
            pygame.mixer_music.play()
