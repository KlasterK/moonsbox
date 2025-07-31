import copy
import dataclasses
from typing import Any, Callable, Literal, final

import pygame

from . import get_font
from .windowevents import BaseEventHandler, StopHandling


@dataclasses.dataclass
class Stylesheet:
    bg_color: pygame.Color = '#ffffff'
    fg_color: pygame.Color = '#000000'
    font: pygame.Font = dataclasses.field(default_factory=get_font)
    bg_image: pygame.Surface | None = None
    bg_position: Literal['top', 'left', 'bottom', 'right', 'center'] = 'center'
    spacing: float = 0

    def copy(self, **changes) -> 'Stylesheet':
        return copy.replace(self, **changes)


@dataclasses.dataclass
class AnchoredPosition:
    x: float
    y: float
    x_anchor: Literal['left', 'center', 'right'] = 'left'
    y_anchor: Literal['top', 'center', 'bottom'] = 'top'

    def create_rect(self, size: pygame.typing.Point) -> pygame.Rect:
        rect = pygame.Rect((0, 0), size)
        screen_w, screen_h = pygame.display.get_window_size()

        match self.x_anchor:
            case 'left':
                rect.left = self.x
            case 'center':
                rect.centerx = self.x + screen_w / 2
            case 'right':
                rect.right = self.x + screen_w
        match self.y_anchor:
            case 'top':
                rect.top = self.y
            case 'center':
                rect.centery = self.y + screen_h / 2
            case 'bottom':
                rect.bottom = self.y + screen_h
        return rect


class PushButton(BaseEventHandler):
    text: str
    pos: AnchoredPosition | None
    idle_style: Stylesheet
    hovered_style: Stylesheet
    pressed_style: Stylesheet
    _cur_style: Stylesheet

    def __init__(
        self,
        *,
        text='',
        pos=None,
        idle_style,
        hovered_style=None,
        pressed_style=None,
    ):
        self._on_click_cbs = []
        self._cur_style = idle_style
        self.text = text  # activating property setter
        self.pos = pos
        self.idle_style = idle_style
        self.hovered_style = idle_style if hovered_style is None else hovered_style
        self.pressed_style = hovered_style if pressed_style is None else pressed_style

    @property
    def text(self):
        return self._text

    @text.setter
    def text(self, v):
        self._text = v
        if v:
            self._rendered_text = self._cur_style.font.render(v, True, self._cur_style.fg_color)
        else:
            self._rendered_text = pygame.Surface((0, 0))

    def connect_on_click(self, cb: Callable[[], Any]) -> None:
        self._on_click_cbs.append(cb)

    def disconnect_on_click(self, cb: Callable) -> None:
        self._on_click_cbs.remove(cb)

    def get_rect(self) -> pygame.Rect:
        if self.pos is None:
            return pygame.Rect(0, 0, 0, 0)

        size = pygame.Vector2(self._cur_style.spacing * 2)
        txt_w, txt_h = self._rendered_text.get_size()

        if self._cur_style.bg_image is not None:
            img_w, img_h = self._cur_style.bg_image.get_size()
            size.x += (
                max(img_w, txt_w)
                if self.pos.x_anchor == 'center'
                else img_w + txt_w + self._cur_style.spacing
            )
            size.y += (
                max(img_h, txt_h)
                if self.pos.y_anchor == 'center'
                else img_h + txt_h + self._cur_style.spacing
            )
        else:
            size.x += txt_w
            size.y += txt_h

        return self.pos.create_rect(size)

    def render(self, dst: pygame.Surface) -> None:
        rt = self._rendered_text
        bg_img = self._cur_style.bg_image
        spc = self._cur_style.spacing
        rect = self.get_rect()
        dst.fill(self._cur_style.bg_color, rect)

        if not self._text:
            if bg_img is not None:
                dst.blit(bg_img, bg_img.get_rect(center=rect.center))
        else:
            if bg_img is not None:
                match self._cur_style.bg_position:
                    case 'center':
                        dst.blit(bg_img, bg_img.get_rect(center=rect.center))
                        dst.blit(rt, rt.get_rect(center=rect.center))

                    case 'top':
                        dst.blit(bg_img, bg_img.get_rect(centerx=rect.centerx, y=rect.y + spc))
                        dst.blit(rt, rt.get_rect(centerx=rect.centerx, bottom=rect.bottom - spc))

                    case 'bottom':
                        dst.blit(
                            bg_img, bg_img.get_rect(centerx=rect.centerx, bottom=rect.bottom - spc)
                        )
                        dst.blit(rt, rt.get_rect(centerx=rect.centerx, y=rect.y + spc))

                    case 'left':
                        dst.blit(bg_img, bg_img.get_rect(x=rect.x + spc, centery=rect.centery))
                        dst.blit(rt, rt.get_rect(right=rect.right - spc, centery=rect.centery))

                    case 'right':
                        dst.blit(
                            bg_img, bg_img.get_rect(right=rect.right - spc, centery=rect.centery)
                        )
                        dst.blit(rt, rt.get_rect(x=rect.x + spc, centery=rect.centery))

            else:
                dst.blit(rt, rt.get_rect(center=rect.center))

    def process_event(self, e):
        if e.type == pygame.MOUSEMOTION and self._cur_style is not self.pressed_style:
            if self.get_rect().collidepoint(e.pos):
                self._cur_style = self.hovered_style
            elif self._cur_style:
                self._cur_style = self.idle_style

        elif e.type == pygame.MOUSEBUTTONDOWN and e.button == pygame.BUTTON_LEFT:
            if self.get_rect().collidepoint(e.pos):
                self._cur_style = self.pressed_style

        elif e.type == pygame.MOUSEBUTTONUP and e.button == pygame.BUTTON_LEFT:
            if self._cur_style is self.pressed_style:
                if self.get_rect().collidepoint(e.pos):
                    self._cur_style = self.hovered_style
                    for cb in self._on_click_cbs:
                        cb()
                else:
                    self._cur_style = self.idle_style

        if self._cur_style is self.pressed_style:
            raise StopHandling
