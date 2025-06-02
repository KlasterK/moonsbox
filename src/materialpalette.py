from typing import TYPE_CHECKING

import pygame

from . import (
    MATERIAL_TRANSLATIONS,
    PALETTE_SELECTION_INNER_COLOR,
    PALETTE_SELECTION_OUTER_COLOR,
    PALETTE_SHADOW_COLOR,
    blend,
    get_font,
    get_material_icon,
)
from .materials import available_materials

if TYPE_CHECKING:
    from .gameapp import GameApp


class MaterialPalette:
    def __init__(
        self,
        icon_size: tuple[int, int],
        max_caption_chars: int,
        margin: int,
        dest: pygame.Surface,
    ):
        self._font = get_font()
        self._margin = margin
        self._max_caption_chars = max_caption_chars
        self._icon_size = icon_size
        self._dest = dest
        self._selected_index = 0
        self._current_row = 0
        self._is_visible = False

        # Prepare material buttons: (button_surface, caption, material_cls)
        self._materials = []

        for name, material_cls in available_materials.items():
            # Prepare icon
            icon = get_material_icon(name)

            if icon:
                icon = pygame.transform.smoothscale(icon, self._icon_size)
            else:
                icon = pygame.Surface(self._icon_size, pygame.SRCALPHA)

            # Prepare caption (truncate if needed)
            caption_text = MATERIAL_TRANSLATIONS.get(name, name)
            if len(caption_text) > self._max_caption_chars:
                caption_text = caption_text[: self._max_caption_chars - 1] + '…'
            caption_surf = self._font.render(caption_text, True, 'white')

            # Compose button surface
            w, h = self._icon_size
            btn_w = max(w, caption_surf.get_width())
            btn_h = h + caption_surf.get_height() + 4

            btn_surf = pygame.Surface((btn_w, btn_h), pygame.SRCALPHA)
            btn_surf.blit(icon, (btn_w // 2 - w // 2, 0))
            btn_surf.blit(caption_surf, (btn_w // 2 - caption_surf.get_width() // 2, h + 2))

            self._materials.append((btn_surf, caption_text, material_cls))

        self.selected_material = self._materials[0][2]

    def __getitem__(self, index: tuple[int, int]):
        cols, _ = self.grid_size
        flat_index = index[1] * cols + index[0]

        if 0 <= flat_index < len(self._materials):
            return self._materials[flat_index][2]
        else:
            return None

    @property
    def grid_size(self) -> tuple[int, int]:
        # Calculate how many buttons fit horizontally and vertically
        screen_w, screen_h = self._dest.get_size()
        btn_w = self._icon_size[0] + self._margin
        btn_h = self._icon_size[1] + self._font.get_height() + 4 + self._margin
        cols = max(1, screen_w // btn_w)
        rows = max(1, screen_h // btn_h)
        return (cols, rows)

    @property
    def selection_slot(self) -> tuple[int, int]:
        cols, _ = self.grid_size
        return (self._selected_index % cols, self._selected_index // cols)

    @selection_slot.setter
    def selection_slot(self, value: tuple[int, int]) -> None:
        cols, _ = self.grid_size
        idx = (self._current_row + value[1]) * cols + value[0]
        if 0 <= idx < len(self._materials):
            self._selected_index = idx
            self.selected_material = self._materials[idx][2]

    @property
    def grid_geometry(self) -> dict[tuple[int, int], pygame.Rect]:
        '''A dictionary with keys as slot positions
        and values as onscreen pygame.Rect for each button.
        '''

        cols, rows = self.grid_size
        btn_w = self._icon_size[0] + self._margin
        btn_h = self._icon_size[1] + self._font.get_height() + self._margin

        screen_w, screen_h = self._dest.get_size()
        total_w = cols * btn_w - self._margin
        total_h = rows * btn_h - self._margin

        x0 = (screen_w - total_w) // 2
        y0 = (screen_h - total_h) // 2

        geometry = {}
        for i in range(rows):
            for j in range(cols):
                idx = (self._current_row + i) * cols + j
                if idx >= len(self._materials):
                    continue

                x = x0 + j * btn_w
                y = y0 + i * btn_h
                geometry[(j, i)] = pygame.Rect(x, y, btn_w, btn_h)

        return geometry

    def go_to_starting_with(self, text: str) -> None:
        text = text.lower()

        for i, (_, name, _) in enumerate(self._materials):
            if name.lower().startswith(text):
                self._selected_index = i
                break

    def show(self) -> None:
        self._is_visible = True
        self._selected_index = 0
        self._current_row = 0

    def hide(self, confirmation: bool) -> None:
        self._is_visible = False
        if confirmation and self._materials:
            self.selected_material = self._materials[self._selected_index][2]

    def is_visible(self) -> bool:
        return self._is_visible

    def render(self) -> None:
        if not self._is_visible:
            return

        # We create a temporary surface because the screen doesn't support transparency
        temp_dest = pygame.Surface(self._dest.get_size(), pygame.SRCALPHA)
        temp_dest.fill(PALETTE_SHADOW_COLOR)

        cols, rows = self.grid_size

        btn_w = self._icon_size[0] + self._margin
        btn_h = self._icon_size[1] + self._font.get_height() + self._margin

        screen_w, screen_h = self._dest.get_size()
        total_w = cols * btn_w - self._margin
        total_h = rows * btn_h - self._margin

        x0 = (screen_w - total_w) // 2
        y0 = (screen_h - total_h) // 2

        for i in range(rows):
            for j in range(cols):
                idx = (self._current_row + i) * cols + j

                if idx >= len(self._materials):
                    continue

                btn_surf, _, _ = self._materials[idx]

                x = x0 + j * btn_w
                y = y0 + i * btn_h

                if idx == self._selected_index:
                    pygame.draw.rect(
                        surface=temp_dest,
                        color=blend(PALETTE_SHADOW_COLOR, PALETTE_SELECTION_INNER_COLOR),
                        rect=(x - 5, y - 5, btn_surf.get_width() + 5, btn_surf.get_height() + 5),
                        width=0,
                        border_radius=8,
                    )

                    pygame.draw.rect(
                        surface=temp_dest,
                        color=blend(PALETTE_SHADOW_COLOR, PALETTE_SELECTION_OUTER_COLOR),
                        rect=(x - 5, y - 5, btn_surf.get_width() + 5, btn_surf.get_height() + 5),
                        width=3,
                        border_radius=8,
                    )

                temp_dest.blit(btn_surf, (x, y))

        # Blit the temporary surface onto the destination
        self._dest.blit(temp_dest, (0, 0))
