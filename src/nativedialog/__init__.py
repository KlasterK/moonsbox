from os import PathLike
import os
import sys
from typing import Iterable, Literal, Sequence
import pygame._sdl2

_IMPL = True
try:
    from ._windows import *
except ImportError:
    # TODO: add native dialog for Apple platforms and X11 without using tkinter
    try:
        from ._tk import *
    except ImportError:
        # TODO: add fallback impl using our UI system
        _IMPL = False

type _BoxKind = Literal['generic', 'error', 'warning', 'info']


def inform(title: str, text: str, kind: _BoxKind = 'generic') -> None:
    pygame._sdl2.messagebox(title, text, None, kind == 'info', kind == 'warning', kind == 'error')


def ask_yes_no(title: str, text: str, kind: _BoxKind = 'generic') -> bool:
    user_input = pygame._sdl2.messagebox(
        title,
        text,
        None,
        kind == 'info',
        kind == 'warning',
        kind == 'error',
        ('Yes', 'No'),
        0,
        1,
    )
    return user_input == 0


def ask_open_file(
    title: str,
    file_types: dict[str, str],
    initial_dir: PathLike = '.',
) -> str:
    if _IMPL:
        return impl_ask_open_file(title, file_types, initial_dir)

    yes = ask_yes_no(
        title=f'{title} - Open File Warning',
        text='We could not find any way to show a native file dialog,'
        'so we are going to use console input to get file path.'
        '\nIf you are unable to input text into console, then the application'
        ' will freeze FOREVER.'
        '\n\nCurrent working directory:'
        f'\n{os.getcwd()}'
        '\n\nFile types:\n' + '\n'.join(f'{k} ({v})' for k, v in file_types) + '\n\nProceed?',
        kind='warning',
    )
    return input(f'{title} - enter file path: ') if yes else ''


def ask_save_file(
    title: str,
    file_types: dict[str, str],
    initial_dir: str = '.',
) -> str:
    if _IMPL:
        return impl_ask_save_file(title, file_types, initial_dir)

    yes = ask_yes_no(
        title=f'{title} - Save File Warning',
        text='We could not find any way to show a native file dialog,'
        'so we are going to use console input to get file path.'
        '\nIf you are unable to get into console, then the application'
        ' will freeze FOREVER.'
        '\n\nCurrent working directory:'
        f'\n{os.getcwd()}'
        '\n\nFile types:\n' + '\n'.join(f'{k} ({v})' for k, v in file_types) + '\n\nProceed?',
        kind='warning',
    )
    return input(f'{title} - enter file path: ') if yes else ''
