# moonsbox.spec
from PyInstaller.utils.hooks import collect_data_files, collect_submodules
import os

a = Analysis(
    ['pyinst_launcher.py'],
    pathex=[os.getcwd()],
    binaries=[],
    datas=[
        ('assets/*', 'assets'),
        ('user/config.toml', 'user'),
    ],
    hiddenimports=[
        # *collect_submodules('src'),
        'src',
        'nativedialog',
        'pygame_ce',
        'numpy',
        'PIL',
        'toml',
    ],
    hookspath=[],
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=None,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=None)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    name='moonsbox',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    icon='assets/window_icon',
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
