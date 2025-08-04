import copy
from abc import ABC, abstractmethod
from dataclasses import MISSING, dataclass, field
from types import EllipsisType as ET
from typing import Any, Generator, Literal, Optional, override

import pygame

from .util import get_font
from .windowevents import BaseEventHandler, StopHandling


@dataclass
class Ruleset:
    bg_color: pygame.Color | ET = ...
    fg_color: pygame.Color | ET = ...
    font: pygame.Font | ET = ...
    spacing: float | ET = ...
    border_width: float | ET = ...
    border_color: pygame.Color | ET = ...
    border_radius: float | ET = ...
    border_topleft_radius: float | ET = ...
    border_topright_radius: float | ET = ...
    border_bottomleft_radius: float | ET = ...
    border_bottomright_radius: float | ET = ...

    @classmethod
    def default(cls) -> 'Ruleset':
        return cls('#00ff00', '#ffffff', get_font(), 10, 0, '#006600', 0, -1, -1, -1, -1)

    def copy(self, *overlays: 'Ruleset', **changes: Any) -> 'Ruleset':
        new_ruleset = copy.copy(self)
        new_ruleset.combine(*overlays, **changes)
        return new_ruleset

    def combine(self, *overlays: 'Ruleset', **changes: Any) -> None:
        for lay in overlays:
            for k in lay.__dataclass_fields__.keys():
                lay_field = getattr(lay, k, ...)
                if lay_field is not ...:
                    setattr(self, k, lay_field)
        for k, v in changes.items():
            setattr(self, k, v)


@dataclass
class Selector:
    id: Any | None = None
    class_name: str = ''
    pseudo: str = ''
    ruleset: Ruleset = field(default_factory=Ruleset)

    # for sorting purposes
    def __lt__(self, other):
        # if self have no X and other have X, then self < other
        return (
            self.id is None
            and other.id is not None
            or not self.pseudo
            and other.pseudo
            or not self.class_name
            and other.class_name
        )

    def test(self, id, class_name, pseudo):
        if self.id is not None:
            if self.id != id:
                return False
        if self.class_name:
            if self.class_name != class_name:
                return False
        if self.pseudo:
            if self.pseudo != pseudo:
                return False
        return True


class Stylesheet:
    def __init__(self, *selectors: Selector):
        self._selectors = sorted(selectors)

    def add(self, *selectors: Selector) -> None:
        self._selectors = sorted((*self._selectors, *selectors))

    def remove(self, id: Any | None = None, class_name: str = '', pseudo: str = '') -> None:
        for i, selec in enumerate(self._selectors):
            if selec.id == id and selec.class_name == class_name and selec.pseudo == pseudo:
                del self._selectors[i]

    def iter(self) -> Generator[Selector]:
        for selec in self._selectors:
            yield selec

    def copy(self) -> 'Stylesheet':
        return Stylesheet(*self._selectors)


@dataclass
class SizePolicy:
    min_w: float
    min_h: float
    w_policy: Literal['min', 'max', 'fixed']
    h_policy: Literal['min', 'max', 'fixed']


class Widget(BaseEventHandler):
    is_visible = True
    size_policy = SizePolicy(0, 0, 'min', 'min')
    capture_surface = None
    id = None
    pseudo = ''

    def __init__(self, parent: 'Container | None' = None, style: Stylesheet = Stylesheet()):
        self._rect = pygame.Rect(0, 0, 0, 0)
        self._style = None
        self._parent = None
        self._captured_surf = None
        # activating properties
        self.style = style
        self.parent = parent

    @property
    def style(self) -> Stylesheet:
        return self._style

    @style.setter
    def style(self, v: Stylesheet) -> None:
        if v is None:
            self._style = Stylesheet()
        else:
            self._style = v.copy()

    @property
    def inherit_style(self) -> Stylesheet:
        if self.parent is None:
            return self.style.copy()
        return Stylesheet(*self.parent.inherit_style.iter(), *self.style.iter())

    @property
    def parent(self) -> 'Container':
        return self._parent

    @parent.setter
    def parent(self, v: 'Container') -> None:
        if self._parent is not None:
            self._parent.remove_child(self)
        if v is not None:
            v.add_child(self)

    # made as a method to remove confusing about returning a copy.
    def get_rect(self, **attrs: Any) -> pygame.Rect:
        if self.capture_surface is None:
            rect = self._rect.copy()
            for k, v in attrs.items():
                setattr(rect, k, v)
        else:
            rect = self.capture_surface.get_rect(**attrs)

        return rect

    def set_rect(self, rect: pygame.Rect | None = None, **attrs: Any) -> None:
        if self.capture_surface is not None:
            return  # makes no sense to set rect

        if rect is not None:
            self._rect.update(rect)
        for k, v in attrs.items():
            setattr(self._rect, k, v)

    def calc_ruleset(self, pseudo: str | None = None) -> Ruleset:
        ruleset = Ruleset.default()
        if pseudo is None:
            pseudo = self.pseudo

        ls = []

        for selec in self.inherit_style.iter():
            ls.append(selec)
            if selec.test(self.id, type(self).__name__, pseudo):
                ruleset.combine(selec.ruleset)

        return ruleset

    def process_event(self, e):
        pass

    def draw(self, dst: pygame.Surface) -> None:
        if not self.is_visible:
            return

        rs = self.calc_ruleset()
        pygame.draw.rect(
            dst,
            rs.bg_color,
            self.get_rect(),
            0,
            rs.border_radius,
            rs.border_topleft_radius,
            rs.border_topright_radius,
            rs.border_bottomleft_radius,
            rs.border_bottomright_radius,
        )
        if rs.border_width > 0:
            pygame.draw.rect(
                dst,
                rs.border_color,
                self.get_rect(),
                rs.border_width,
                rs.border_radius,
                rs.border_topleft_radius,
                rs.border_topright_radius,
                rs.border_bottomleft_radius,
                rs.border_bottomright_radius,
            )


class Container(Widget):
    def __init__(self, parent=None, style=None):
        super().__init__(parent, style)
        self._children: list[Widget] = []
        self._central_child = None

    def add_child(self, child):
        self._children.append(child)
        child._parent = self

    def remove_child(self, child):
        self._children.remove(child)  # may raise
        child._parent = None

    def set_central_widget(self, child):
        if child not in self._children:
            self._children.append(child)
        self._central_child = child

    def move_to_top(self, child):
        if child in self._children:
            self._children.remove(child)
        self._children.append(child)

    def process_event(self, e):
        if not self.is_visible:
            return

        for child in reversed(self._children):  # lastly added widgets will get events first
            child.process_event(e)  # may raise StopHandling

    def draw(self, dst):
        if not self.is_visible:
            return

        if self._central_child is not None:
            # stretch the central child on the whole container
            self._central_child.set_rect(self.get_rect())

        for child in self._children:  # firstly added widget will be drew first
            child.draw(dst)


class HBoxLayout(Container):
    def draw(self, dst):
        if not self.is_visible:
            return

        # TODO: not recalculate all rects on every frame
        spacing = self.calc_ruleset().spacing
        my_rect = self.get_rect()

        # Calculating the widths.
        policies = [w.size_policy for w in self._children]
        if all(p.w_policy in ('min', 'max') for p in policies):
            # Then we will just split up the surface on equal parts
            # TODO: take into account minimal widths
            mid_w = (my_rect.w - spacing) / len(policies) - spacing
            widths = [mid_w] * len(policies)
        else:
            remaining_w = my_rect.w - spacing * 2
            widths = []

            for p in policies:
                if p.w_policy == 'max':
                    widths.append(None)
                else:
                    widths.append(p.min_w)
                    remaining_w -= p.min_w

                    if remaining_w < 0:
                        widths[-1] += remaining_w
                        remaining_w = 0
                        break  # TODO: take into account minimal widths, and not just interrupt the calculating

            # TODO: give higher priority to 'max' size policy
            count_of_max = policies.count(None)
            if count_of_max > 0:
                mid_w = (remaining_w - spacing) / len(policies) - spacing
                for i, value in enumerate(widths):
                    if value is None:
                        widths[i] = mid_w

        # Placing the widgets.
        in_rect = my_rect.inflate(-spacing * 2, -spacing * 2)
        x_offset = 0
        for width, policy, widget in zip(widths, policies, self._children):
            if policy.h_policy == 'fixed':
                height = min(policy.min_h, in_rect.h)
            else:
                height = in_rect.h

            widget.set_rect(x=in_rect.x + x_offset, y=in_rect.y, w=width, h=height)
            x_offset += width + spacing

        super().draw(dst)


class Subwindow(Container):
    def __init__(self, title, parent=None, style=None):
        super().__init__(parent, style)
        self.title = title
        self._title_rect = pygame.Rect(0, 0, 0, 0)
        self._content_rect = pygame.Rect(0, 0, 0, 0)
        self._is_title_captured = False

    @property
    def size_policy(self):
        return SizePolicy(self._title_rect.w, self._title_rect.h, 'max', 'max')

    def draw(self, dst: pygame.Surface):
        if not self.is_visible:
            return

        rs = self.calc_ruleset()
        my_rect = self.get_rect()

        # Drawing the window title.
        # NOTE: if title='', then the surface is sized 0xLineHeight
        title_text_surf = rs.font.render(self.title, True, rs.fg_color)
        title_bg_rect = pygame.Rect(
            my_rect.x, my_rect.y, my_rect.w, title_text_surf.size[1] + rs.spacing * 2
        )
        pygame.draw.rect(
            dst,
            rs.border_color,
            title_bg_rect,
            0,
            rs.border_radius,
            rs.border_topleft_radius,
            rs.border_topright_radius,
            0,
            0,
        )
        content_rect = my_rect.copy()
        content_rect.y += title_bg_rect.h
        content_rect.h -= title_bg_rect.h
        pygame.draw.rect(
            dst,
            rs.bg_color,
            content_rect,
            0,
            rs.border_radius,
            0,
            0,
            rs.border_bottomleft_radius,
            rs.border_bottomright_radius,
        )
        if rs.border_width > 0:
            pygame.draw.rect(
                dst,
                rs.border_color,
                content_rect,
                rs.border_width,
                rs.border_radius,
                0,
                0,
                rs.border_bottomleft_radius,
                rs.border_bottomright_radius,
            )

        title_text_rect = title_text_surf.get_rect(center=title_bg_rect.center)
        dst.blit(title_text_surf, title_text_rect)

        if self._central_child is not None:
            self._central_child.set_rect(content_rect)

        # TODO: clip rounded borders
        dst.set_clip(content_rect)
        for child in self._children:
            child.draw(dst)
        dst.set_clip(None)

        # Save calculated rects for event handling
        self._title_rect = title_bg_rect
        self._content_rect = content_rect

    def process_event(self, e):
        if not self.is_visible:
            return

        if (
            e.type == pygame.MOUSEBUTTONDOWN
            and e.button == pygame.BUTTON_LEFT
            and self._title_rect.collidepoint(e.pos)
        ):
            self._is_title_captured = True
            raise StopHandling

        elif e.type == pygame.MOUSEBUTTONUP and e.button == pygame.BUTTON_LEFT:
            self._is_title_captured = False

        elif e.type == pygame.MOUSEMOTION and self._is_title_captured:
            for widget in (self, *self._children):
                rect = widget.get_rect()
                widget.set_rect(x=rect.x + e.rel[0], y=rect.y + e.rel[1])
            raise StopHandling

        if 'pos' in e.dict and not self._content_rect.collidepoint(e.pos):
            return  # hack to prevent forwarding events to clipped parts of children

        for widget in reversed(self._children):  # lastly added widgets will get events first
            widget.process_event(e)  # may raise StopHandling


class Label(Widget):
    text: str
    image: pygame.Surface | None
    image_pos: Literal['center', 'top', 'left', 'bottom', 'right']

    def __init__(self, text='', image=None, image_pos='center', parent=None, style=None):
        self.text = text
        self.image = image
        self.image_pos = image_pos
        self._user_size_policy = None
        super().__init__(parent, style)

    @property
    def size_policy(self):
        rs = self.calc_ruleset()
        txt_w, txt_h = rs.font.size(self.text) if self.text else (0, 0)
        img_w, img_h = self.image.size if self.image is not None else (0, 0)

        if self.image_pos in ('left', 'right'):
            w, h = txt_w + rs.spacing + img_w, max(img_h, txt_h)
        elif self.image_pos in ('top', 'bottom'):
            w, h = max(img_w, txt_w), txt_h + rs.spacing + img_h
        else:  # center
            w, h = max(txt_w, img_w), max(txt_h, img_h)

        policy = SizePolicy(w + rs.spacing * 2, h + rs.spacing * 2, 'min', 'min')

        if self._user_size_policy is not None:
            policy.min_w = max(policy.min_w, self._user_size_policy.min_w)
            policy.min_h = max(policy.min_h, self._user_size_policy.min_h)
            policy.w_policy = self._user_size_policy.w_policy
            policy.h_policy = self._user_size_policy.h_policy
        return policy

    @size_policy.setter
    def size_policy(self, v):
        self._user_size_policy = v

    def draw(self, dst: pygame.Surface):
        if not self.is_visible:
            return

        super().draw(dst)
        rs = self.calc_ruleset()
        inner_rect = self.get_rect().inflate(-rs.spacing * 2, -rs.spacing * 2)

        if self.text:
            text_surf = rs.font.render(self.text, True, rs.fg_color)
        else:
            text_surf = pygame.Surface((0, 0))

        if self.image is not None:
            image_surf = self.image
        else:
            image_surf = pygame.Surface((0, 0))

        match self.image_pos:
            case 'center':
                text_rect = text_surf.get_rect(center=inner_rect.center)
                image_rect = image_surf.get_rect(center=inner_rect.center)
            case 'top':
                text_rect = text_surf.get_rect(centerx=inner_rect.centerx, bottom=inner_rect.bottom)
                image_rect = image_surf.get_rect(centerx=inner_rect.centerx, y=inner_rect.y)
            case 'bottom':
                text_rect = text_surf.get_rect(centerx=inner_rect.centerx, y=inner_rect.y)
                image_rect = image_surf.get_rect(
                    centerx=inner_rect.centerx, bottom=inner_rect.bottom
                )
            case 'left':
                text_rect = text_surf.get_rect(x=inner_rect.x, centery=inner_rect.centery)
                image_rect = image_surf.get_rect(right=inner_rect.right, centery=inner_rect.centery)
            case 'right':
                text_rect = text_surf.get_rect(right=inner_rect.right, centery=inner_rect.centery)
                image_rect = image_surf.get_rect(x=inner_rect.x, centery=inner_rect.centery)

        dst.blit(image_surf, image_rect)
        dst.blit(text_surf, text_rect)


class Button(Label):
    def __init__(self, text='', image=None, image_pos='center', cb=None, parent=None, style=None):
        self._cbs = []
        self._pseudo = ''
        if cb is not None:
            self._cbs.append(cb)
        super().__init__(text, image, image_pos, parent, style)

    @property
    def pseudo(self):
        return self._pseudo

    @pseudo.setter
    def pseudo(self, v):
        old_value = self._pseudo
        self._pseudo = v
        for cb in self._cbs:
            cb(self, old_value)

    def add_cb(self, cb):
        self._cbs.append(cb)

    def remove_cb(self, cb):
        self._cbs.remove(cb)

    def process_event(self, e):
        if not self.is_visible:
            return

        if e.type == pygame.MOUSEMOTION:
            col = self.get_rect().collidepoint(e.pos)
            if not self.pseudo and col:
                self.pseudo = 'hover'
            elif self.pseudo == 'hover' and not col:
                self.pseudo = ''
            elif self.pseudo == 'pressed':
                raise StopHandling

        elif e.type == pygame.MOUSEBUTTONDOWN and e.button == pygame.BUTTON_LEFT:
            if self.get_rect().collidepoint(e.pos):
                self.pseudo = 'pressed'
                raise StopHandling

        elif e.type == pygame.MOUSEBUTTONUP and e.button == pygame.BUTTON_LEFT:
            if self.pseudo == 'pressed':
                if self.get_rect().collidepoint(e.pos):
                    self.pseudo = 'hover'
                    raise StopHandling
                else:
                    self.pseudo = ''
