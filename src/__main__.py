import pygame

from . import ASSETS_ROOT, MUSIC_VOLUME
from .gameapp import GameApp

pygame.init()

pygame.mixer_music.load(ASSETS_ROOT / 'sounds' / 'ambient')
pygame.mixer_music.set_volume(MUSIC_VOLUME)
pygame.mixer_music.play(-1)

app = GameApp()
app.run()

pygame.quit()
