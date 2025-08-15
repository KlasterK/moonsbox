import math
import traceback

import pygame

from .config import (
    DEBUG_COLOR,
    ENABLE_FPS_COUNTER,
    ENABLE_TPS_COUNTER,
    ENABLE_VSYNC,
    FONT_SIZE,
    MAP_INNER_COLOR,
    MAP_OUTER_COLOR,
    MAP_SIZE,
    PALETTE_ICON_SIZE,
    PALETTE_MARGIN,
    PALETTE_MAX_CAPTION_CHARS,
    SCREEN_SIZE,
    VISIBLE_AREA,
    WINDOW_CAPTION,
)
from .gamemap import GameMap
from .materialpalette import MaterialPalette
from .materials import Sand
from .renderer import Renderer
from .simulation import SimulationManager
from .ui import (
    Button,
    HBoxLayout,
    Ruleset,
    Selector,
    SizePolicy,
    Stylesheet,
    Container,
    Subwindow,
)
from .util import GameSound, get_font, get_image
from .windowevents import (
    CameraEventHandler,
    CapturingEventHandler,
    DrawingEventHandler,
    GameAppEventHandler,
    MaterialPaletteEventHandler,
    SimulationEventHandler,
    StopHandling,
)
from . import nativedialog


class Camera:
    def __init__(
        self,
        rendering_target: pygame.Surface,
        game_map: GameMap,
        default_visible_area: pygame.Rect,
    ):
        self._map = game_map
        self._surface = rendering_target

        self._pos = default_visible_area.topleft

        screen_w, screen_h = self._surface.get_size()
        area_w, area_h = default_visible_area.size
        self._scale = pygame.Vector2(area_w / screen_w, area_h / screen_h)

    def zoom(self, factor: float) -> None:
        # Calculate zoom multiplier
        if factor > 0:
            multiplier = factor + 1
        else:
            multiplier = 1 / (-factor + 1)

        # Current scale and position
        old_scale = pygame.Vector2(self._scale)
        new_scale = old_scale * multiplier

        # Mouse position on screen (in pixels)
        mx, my = pygame.mouse.get_pos()

        # Calculate the world position under the mouse before zoom
        world_x = mx * old_scale.x + self._pos[0]
        world_y = my * old_scale.y + self._pos[1]

        # After zoom, adjust camera position so the same world point stays under the mouse
        new_pos_x = world_x - mx * new_scale.x
        new_pos_y = world_y - my * new_scale.y

        self._scale = (new_scale.x, new_scale.y)
        self._pos = (new_pos_x, new_pos_y)

    def move(self, rel_x: int, rel_y: int) -> None:
        x = self._pos[0] + rel_x * self._scale[0]
        y = self._pos[1] + rel_y * self._scale[1]
        self._pos = x, y

    def get_area(self) -> pygame.Rect:
        screen_w, screen_h = self._surface.get_size()
        area_size = screen_w * self._scale[0], screen_h * self._scale[1]

        area = pygame.Rect(self._pos, area_size)
        return area

    def convert_pos(self, onscreen_pos: tuple[int, int]) -> tuple[int, int]:
        absolute_pos = (
            math.floor(onscreen_pos[0] * self._scale[0] + self._pos[0] + 0.5),
            math.floor(onscreen_pos[1] * self._scale[1] + self._pos[1] + 0.5),
        )

        return absolute_pos


class GameApp:
    def __init__(self):
        if not pygame.get_init():
            raise RuntimeError('pygame is not initialised')

        pygame.display.set_mode(
            SCREEN_SIZE,
            pygame.RESIZABLE | pygame.DOUBLEBUF | pygame.HWSURFACE | pygame.HWACCEL,
            vsync=ENABLE_VSYNC,
        )
        pygame.display.set_caption(WINDOW_CAPTION)
        pygame.display.set_icon(get_image('window_icon'))

        self._screen = pygame.display.get_surface()

        self._is_running = True
        self._is_paused = False

        self._map = GameMap(MAP_SIZE)
        self._sim = SimulationManager(self._map)

        self._camera = Camera(self._screen, self._map, VISIBLE_AREA)
        self._renderer = Renderer(self._map, self._screen, MAP_INNER_COLOR)

        self._pal = MaterialPalette(
            PALETTE_ICON_SIZE,
            PALETTE_MAX_CAPTION_CHARS,
            PALETTE_MARGIN,
            self._screen,
        )
        self._pal.selected_material = Sand

        style = Stylesheet(
            Selector(
                class_name='Button',
                ruleset=Ruleset(bg_color='#7C6423'),
            ),
            Selector(
                class_name='Button',
                pseudo='hover',
                ruleset=Ruleset(
                    bg_color='#63511F',
                ),
            ),
            Selector(
                class_name='Button',
                pseudo='pressed',
                ruleset=Ruleset(bg_color="#A98930"),
            ),
        )
        self._ui = Container(None, style)
        self._ui.capture_surface = self._screen
        layout = HBoxLayout(self._ui)
        self._ui.set_central_widget(layout)

        btn_materials = Button(parent=layout, image=get_image('materials_palette_btn_icon'))
        btn_materials.size_policy = SizePolicy(-1, -1, 'fixed', 'fixed')

        @btn_materials.add_cb
        def _(_, old):
            if btn_materials.pseudo == 'hover' and old == 'pressed':
                GameSound('palette.show').play_override()
                self._pal.show()

        btn_saving = Button(parent=layout, image=get_image('saving_btn_icon'))
        btn_saving.size_policy = SizePolicy(-1, -1, 'fixed', 'fixed')

        @btn_saving.add_cb
        def _(_, old):
            if btn_saving.pseudo != 'hover' or old != 'pressed':
                return
            w, h = self._screen.size
            sub = Subwindow('Saving', self._ui)
            sub.set_rect(w=w / 2, h=h / 5, centerx=w / 2, centery=h / 2)
            layout = HBoxLayout(sub)
            sub.set_central_widget(layout)

            btn_load = Button('Load', parent=layout)
            btn_save = Button('Save', parent=layout)
            btn_close = Button('Cancel', parent=layout)

            @btn_load.add_cb
            def _(_, old):
                if btn_load.pseudo != 'hover' or old != 'pressed':
                    return
                try:
                    file_types = {'Moonsbox Save': '*.kk-save', 'All Files': '*.*'}
                    file_name = nativedialog.ask_open_file('Load Save', file_types, 'user')
                    if file_name:
                        with open(file_name, 'rb') as file:
                            self._map.load(file)
                except (IOError, ValueError) as e:
                    nativedialog.inform(
                        'Load Error',
                        f'Failed to load the save!\nSystem info: {type(e).__name__}: {e!s}',
                        'warning',
                    )
                finally:
                    sub.parent = None  # should delete the subwindow

            @btn_save.add_cb
            def _(_, old):
                if btn_save.pseudo != 'hover' or old != 'pressed':
                    return
                try:
                    file_types = {'Moonsbox Save': '*.kk-save', 'All Files': '*.*'}
                    file_name = nativedialog.ask_save_file(
                        'Save Map', file_types, 'user', '.kk-save'
                    )
                    if file_name:
                        with open(file_name, 'wb') as file:
                            self._map.dump(file)
                except (IOError, ValueError) as e:
                    nativedialog.inform(
                        'Save Error',
                        f'Failed to save the map!\nSystem info: {type(e).__name__}: {e!s}',
                        'warning',
                    )
                finally:
                    sub.parent = None

            @btn_close.add_cb
            def _(_, old):
                if btn_close.pseudo == 'hover' and old == 'pressed':
                    sub.parent = None

        self._event_handlers = (
            MaterialPaletteEventHandler(self, self._pal),
            self._ui,
            GameAppEventHandler(self),
            SimulationEventHandler(self._map, self._sim),
            CameraEventHandler(self._camera),
            DrawingEventHandler(self._camera, self._map, self._pal),
            CapturingEventHandler(self._renderer),
        )

    def run(self) -> None:
        if not self._is_running:
            raise RuntimeError('game is stopped')

        GameSound('stream.ambient').play()
        font = get_font()

        tps_pos = 10, 10
        fps_pos = 10, 10 + FONT_SIZE + 5 if ENABLE_TPS_COUNTER else 10

        while self._is_running:
            for event in pygame.event.get():
                for handler in self._event_handlers:
                    try:
                        handler.process_event(event)
                    except StopHandling:
                        break

            if not self._is_paused:
                self._sim.tick()

            self._screen.fill(MAP_OUTER_COLOR)
            self._renderer.render(self._camera.get_area())
            self._pal.render()
            if not self._pal.is_visible():
                self._ui.draw(self._screen)

            if ENABLE_TPS_COUNTER:
                self._screen.blit(
                    font.render(f"TPS = {self._sim.get_tps():.2f}", True, DEBUG_COLOR), tps_pos
                )
            if ENABLE_FPS_COUNTER:
                self._screen.blit(
                    font.render(f"FPS = {self._renderer.get_fps():.2f}", True, DEBUG_COLOR), fps_pos
                )

            pygame.display.flip()

    def stop(self) -> None:
        self._is_running = False

    def is_running(self) -> bool:
        return self._is_running

    def set_paused(self, value: bool) -> None:
        self._is_paused = value

    def is_paused(self) -> bool:
        return self._is_paused
