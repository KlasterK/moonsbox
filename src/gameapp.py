import math
import threading
import time

import pygame

from .const import (
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
from .renderer import Renderer, available_render_masks
from .simulation import SimulationManager
from .ui import AnchoredPosition, ButtonContainer, Stylesheet
from .util import GameSound, get_font, get_image
from .windowevents import (
    CameraEventHandler,
    DrawingEventHandler,
    GameAppEventHandler,
    MaterialPaletteEventHandler,
    SimulationEventHandler,
    StopHandling,
)


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
        self._is_rendering = True

        self._map = GameMap(MAP_SIZE)
        self._sim = SimulationManager(self._map)

        self._camera = Camera(self._screen, self._map, VISIBLE_AREA)
        self._renderer = Renderer(self._map, self._screen)
        self._render_mask = available_render_masks[0]
        self._render_mask_index = 0

        self._pal = MaterialPalette(
            PALETTE_ICON_SIZE,
            PALETTE_MAX_CAPTION_CHARS,
            PALETTE_MARGIN,
            self._screen,
        )
        self._pal.selected_material = Sand

        idle_style = Stylesheet(bg_color="#7C6423", spacing=5)
        hovered_style = idle_style.copy(bg_color="#63511F")

        self._buttons = ButtonContainer(idle_style, hovered_style, hovered_style)
        self._buttons.add(
            AnchoredPosition(10, 10),
            image=get_image('materials_palette_btn_icon'),
            cb=lambda: {self._pal.show(), GameSound('palette.show').play_override()},
        )

        self._event_handlers = (
            MaterialPaletteEventHandler(self, self._pal),
            self._buttons,
            GameAppEventHandler(self),
            SimulationEventHandler(self._map, self._sim),
            CameraEventHandler(self._camera),
            DrawingEventHandler(self._camera, self._map, self._pal),
        )

    # from profilehooks import profile
    # @profile(stdout=False, filename='GameApp-run.prof')
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
            if self._is_rendering:
                self._renderer.render(self._camera.get_area(), self._render_mask, MAP_INNER_COLOR)
                self._pal.render()
                if not self._pal.is_visible():
                    self._buttons.render(self._screen)

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

    def next_render_mask(self) -> None:
        self._render_mask_index = (self._render_mask_index + 1) % len(available_render_masks)
        self._render_mask = available_render_masks[self._render_mask_index]

    def is_rendering(self) -> bool:
        return self._is_rendering

    def set_rendering(self, value: bool) -> None:
        self._is_rendering = value
