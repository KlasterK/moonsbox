import pygame
import ctypes
from ctypes import wintypes


class OPENFILENAMEW(ctypes.Structure):
    _fields_ = [
        ('lStructSize', wintypes.DWORD),
        ('hwndOwner', wintypes.HWND),
        ('hInstance', wintypes.HINSTANCE),
        ('lpstrFilter', wintypes.LPCWSTR),
        ('lpstrCustomFilter', wintypes.LPWSTR),
        ('nMaxCustFilter', wintypes.DWORD),
        ('nFilterIndex', wintypes.DWORD),
        ('lpstrFile', wintypes.LPWSTR),
        ('nMaxFile', wintypes.DWORD),
        ('lpstrFileTitle', wintypes.LPWSTR),
        ('nMaxFileTitle', wintypes.DWORD),
        ('lpstrInitialDir', wintypes.LPCWSTR),
        ('lpstrTitle', wintypes.LPCWSTR),
        ('Flags', wintypes.DWORD),
        ('nFileOffset', wintypes.WORD),
        ('nFileExtension', wintypes.WORD),
        ('lpstrDefExt', wintypes.LPCWSTR),
        ('lCustData', wintypes.LPARAM),
        ('lpfnHook', wintypes.LPVOID),
        ('lpTemplateName', wintypes.LPCWSTR),
        ('pvReserved', wintypes.LPVOID),
        ('dwReserved', wintypes.DWORD),
        ('FlagsEx', wintypes.DWORD),
    ]

    @classmethod
    def create(cls, hwnd_owner, ret_file_path_buf, title, initial_dir, filter, flags, def_ext=None):
        ofn = cls()
        ofn.lStructSize = ctypes.sizeof(ofn)
        ofn.hwndOwner = hwnd_owner
        ofn.lpstrFile = ctypes.cast(ret_file_path_buf, wintypes.LPWSTR)
        ofn.nMaxFile = wintypes.MAX_PATH
        ofn.lpstrInitialDir = initial_dir
        ofn.lpstrTitle = title
        ofn.Flags = flags
        ofn.lpstrFilter = filter
        if def_ext is not None:
            ofn.lpstrDefExt = def_ext
        return ofn


OFN_FILEMUSTEXIST = 0x00001000
OFN_PATHMUSTEXIST = 0x00000800
OFN_EXPLORER = 0x00080000

commdlg = ctypes.WinDLL('comdlg32.dll')

GetOpenFileNameW = commdlg.GetOpenFileNameW
GetOpenFileNameW.restype = wintypes.BOOL
GetOpenFileNameW.argtypes = [ctypes.POINTER(OPENFILENAMEW)]

GetSaveFileNameW = commdlg.GetSaveFileNameW
GetSaveFileNameW.restype = wintypes.BOOL
GetSaveFileNameW.argtypes = [ctypes.POINTER(OPENFILENAMEW)]


def impl_ask_open_file(title, file_types, initial_dir):
    ret_file_path_buffer = ctypes.create_unicode_buffer(wintypes.MAX_PATH)

    ofn = OPENFILENAMEW.create(
        pygame.display.get_wm_info().get('window', 0) if pygame.display.get_init() else 0,
        ret_file_path_buffer,
        title,
        initial_dir,
        ''.join(f'{k}\0{v}\0' for k, v in file_types.items()) + '\0',
        OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER,
    )

    if GetOpenFileNameW(ctypes.byref(ofn)):
        return ret_file_path_buffer.value
    return ''


def impl_ask_save_file(title, file_types, initial_dir, default_extension):
    ret_file_path_buffer = ctypes.create_unicode_buffer(wintypes.MAX_PATH)

    ofn = OPENFILENAMEW.create(
        pygame.display.get_wm_info().get('window', 0) if pygame.display.get_init() else 0,
        ret_file_path_buffer,
        title,
        initial_dir,
        ''.join(f'{k}\0{v}\0' for k, v in file_types.items()) + '\0',
        OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER,
        def_ext=default_extension,
    )

    if GetSaveFileNameW(ctypes.byref(ofn)):
        return ret_file_path_buffer.value
    return ''
