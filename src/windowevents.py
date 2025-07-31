import abc
from typing import TYPE_CHECKING

import PIL.Image
import pygame

from . import (
    DEFAULT_DRAWING_WIDTH,
    DELTA_DRAWING_WIDTH,
    DRAWING_IS_CIRCULAR,
    ENABLE_VSYNC,
    SCREEN_SIZE,
    SCREENSHOT_DOT_SIZE,
    SCREENSHOT_PATH_FACTORY,
    SCREENSHOT_TYPE_HINT,
    ZOOM_FACTOR,
    GameSound,
)
from .gamemap import GameMap
from .materialpalette import MaterialPalette
from .materials import Space
from .simulation import SimulationManager

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


class GameAppEventHandler(BaseEventHandler):
    def __init__(self, app: 'GameApp'):
        self._app = app

    def process_event(self, e):
        if e.type == pygame.QUIT:
            self._app.stop()

        elif e.type == pygame.KEYDOWN:
            if e.key == pygame.K_ESCAPE:
                self._app.stop()

            elif e.key == pygame.K_SPACE:  # `~ key
                self._app.set_paused(not self._app.is_paused())

            elif e.key == pygame.K_F1:
                self._app.next_render_mask()

            elif e.key == pygame.K_r:
                self._app.set_rendering(False)
                if ENABLE_VSYNC:
                    pygame.display.set_mode(
                        SCREEN_SIZE,
                        pygame.RESIZABLE | pygame.DOUBLEBUF | pygame.HWSURFACE | pygame.HWACCEL,
                        vsync=0,
                    )

        elif e.type == pygame.KEYUP:
            if e.key == pygame.K_r:
                self._app.set_rendering(True)
                if ENABLE_VSYNC:
                    pygame.display.set_mode(
                        SCREEN_SIZE,
                        pygame.RESIZABLE | pygame.DOUBLEBUF | pygame.HWSURFACE | pygame.HWACCEL,
                        vsync=1,
                    )


class SimulationEventHandler(BaseEventHandler):
    def __init__(self, game_map: GameMap, simulation_manager: SimulationManager):
        self._map = game_map
        self._sim = simulation_manager

    def process_event(self, e):
        if e.type == pygame.KEYDOWN:
            if e.key == pygame.K_F5:
                self._sim.tick()
            elif e.key == pygame.K_F6:
                self._map.fill(Space)
            elif e.key == pygame.K_F9:
                unscaled = self._map.take_screenshot()
                scaled = unscaled.resize(
                    (
                        unscaled.width * SCREENSHOT_DOT_SIZE[0],
                        unscaled.height * SCREENSHOT_DOT_SIZE[1],
                    ),
                    PIL.Image.Resampling.NEAREST,
                )
                scaled.show()
            elif e.key == pygame.K_F10:
                unscaled = self._map.take_screenshot()
                scaled = unscaled.resize(
                    (
                        unscaled.width * SCREENSHOT_DOT_SIZE[0],
                        unscaled.height * SCREENSHOT_DOT_SIZE[1],
                    ),
                    PIL.Image.Resampling.NEAREST,
                )
                scaled.save(SCREENSHOT_PATH_FACTORY(), SCREENSHOT_TYPE_HINT)


class CameraEventHandler(BaseEventHandler):
    def __init__(self, camera: 'Camera'):
        self._camera = camera

    def process_event(self, e):
        if e.type == pygame.MOUSEMOTION:
            mods = pygame.key.get_mods()

            if mods & pygame.KMOD_SHIFT or e.buttons[2]:
                self._camera.move(-e.rel[0], -e.rel[1])

        elif e.type == pygame.MOUSEWHEEL:
            # Changing scale factors
            # self._visible_area_size_scale.x += e.precise_y * 0.1
            # self._visible_area_size_scale.y += e.precise_y * 0.1

            # Saving center to assign it later, we will work with 'radius' of the area
            self._camera.zoom(-e.precise_y * ZOOM_FACTOR)


class DrawingEventHandler(BaseEventHandler):
    def __init__(self, camera: 'Camera', game_map: GameMap, palette: MaterialPalette):
        self._camera = camera
        self._map = game_map
        self._pal = palette
        self._previous_pos = None
        self._width = DEFAULT_DRAWING_WIDTH
        self._is_hold_drawing_event_sent = False

    def process_event(self, e):
        # mods = pygame.key.get_mods()
        # buttons = pygame.mouse.get_pressed(3)

        # if mods & pygame.KMOD_CTRL or buttons[0]:
        #     self._map[
        #         pygame.mouse.get_pos(),
        #         ('convert', self._camera.get_area(), self._screen.get_size()),
        #         '~y',
        #     ] = self._selected_material()

        if (e.type == pygame.KEYDOWN and e.key == pygame.K_LCTRL) or (
            e.type == pygame.MOUSEBUTTONDOWN and e.button == pygame.BUTTON_LEFT
        ):
            mouse_pos = pygame.mouse.get_pos()
            abs_pos = self._map.invy_pos(self._camera.convert_pos(mouse_pos))

            rect = pygame.Rect(0, 0, self._width, self._width)
            rect.center = abs_pos
            if DRAWING_IS_CIRCULAR:
                self._map.draw_ellipse(rect, self._pal.selected_material)
            else:
                self._map.draw_rect(rect, self._pal.selected_material)
            self._previous_pos = abs_pos

            pygame.event.post(pygame.event.Event(pygame.USEREVENT, purpose='hold-drawing'))

        elif e.type == pygame.MOUSEMOTION:
            if self._previous_pos is not None:
                mouse_pos = pygame.mouse.get_pos()
                abs_pos = self._map.invy_pos(self._camera.convert_pos(mouse_pos))

                # Drawing a line is slow so we won't do it if delta motion is too small
                # min_delta = self._width // 3
                # if (
                #     abs(abs_pos[0] - self._previous_pos[0]) < min_delta
                #     or abs(abs_pos[0] - self._previous_pos[0]) < min_delta
                # ):
                #     self._map.
                # else:
                self._map.draw_line(
                    self._previous_pos, abs_pos, self._width, self._pal.selected_material
                )

                rect = pygame.Rect(0, 0, self._width, self._width)
                rect.center = abs_pos

                self._previous_pos = abs_pos

        elif (e.type == pygame.KEYUP and e.key == pygame.K_LCTRL) or (
            e.type == pygame.MOUSEBUTTONUP and e.button == pygame.BUTTON_LEFT
        ):
            self._previous_pos = None

        elif e.type == pygame.KEYDOWN and e.key == pygame.K_EQUALS:
            self._width += DELTA_DRAWING_WIDTH

        elif e.type == pygame.KEYDOWN and e.key == pygame.K_MINUS:
            self._width = max(1, self._width - DELTA_DRAWING_WIDTH)

        elif e.type == pygame.USEREVENT and getattr(e, 'purpose', None) == 'hold-drawing':
            if self._previous_pos is not None:
                rect = pygame.Rect(0, 0, self._width, self._width)
                rect.center = self._previous_pos
                if DRAWING_IS_CIRCULAR:
                    self._map.draw_ellipse(rect, self._pal.selected_material)
                else:
                    self._map.draw_rect(rect, self._pal.selected_material)

                pygame.event.post(pygame.event.Event(pygame.USEREVENT, purpose='hold-drawing'))


class MaterialPaletteEventHandler(BaseEventHandler):
    def __init__(self, game_app: 'GameApp', palette: MaterialPalette):
        self._app = game_app
        self._pal = palette
        self._previous_material = None

    def process_event(self, e):
        if self._pal.is_visible():
            if e.type == pygame.KEYDOWN:
                if e.key == pygame.K_LEFT:
                    x, y = self._pal.selection_slot
                    x = max(x - 1, 0)
                    self._pal.selection_slot = (x, y)
                    GameSound('palette.move_selection').play_override()

                elif e.key == pygame.K_RIGHT:
                    x, y = self._pal.selection_slot
                    width = self._pal.whole_grid_size[0]
                    x = min(x + 1, width - 1)
                    self._pal.selection_slot = (x, y)
                    GameSound('palette.move_selection').play_override()

                elif e.key == pygame.K_UP:
                    x, y = self._pal.selection_slot
                    y = max(y - 1, 0)
                    self._pal.selection_slot = (x, y)
                    GameSound('palette.move_selection').play_override()

                elif e.key == pygame.K_DOWN:
                    x, y = self._pal.selection_slot
                    height = self._pal.whole_grid_size[1]
                    y = min(y + 1, height - 1)
                    self._pal.selection_slot = (x, y)
                    GameSound('palette.move_selection').play_override()

                elif e.key == pygame.K_RETURN:
                    self._pal.hide(True)
                    GameSound('palette.hide_confirmation').play_override()

                elif e.key == pygame.K_ESCAPE:
                    self._pal.hide(False)
                    GameSound('palette.hide_no_confirmation').play_override()

                elif e.unicode and e.unicode.isalpha():
                    self._pal.go_to_starting_with(e.unicode)
                    GameSound('palette.move_selection').play_override()

                raise StopHandling

            elif e.type == pygame.MOUSEBUTTONDOWN:
                if e.button == pygame.BUTTON_LEFT:
                    previous_slot_pos = self._pal.selection_slot

                    for slot_pos, rect in self._pal.grid_geometry.items():
                        if rect.collidepoint(e.pos):
                            self._pal.selection_slot = slot_pos
                            if slot_pos == previous_slot_pos:
                                self._pal.hide(True)
                                GameSound('palette.hide_confirmation').play_override()
                            else:
                                GameSound('palette.move_selection').play_override()
                            break
                    else:
                        # If no slot was clicked, hide the palette
                        self._pal.hide(False)
                        GameSound('palette.hide_no_confirmation').play_override()

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
            if (e.type == pygame.KEYDOWN and e.key == pygame.K_TAB) or (
                e.type == pygame.MOUSEBUTTONDOWN and e.button == pygame.BUTTON_MIDDLE
            ):
                self._pal.show()
                GameSound('palette.show').play_override()
