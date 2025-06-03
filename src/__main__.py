import pygame

from . import ASSETS_ROOT, MUSIC_VOLUME
from .gameapp import GameApp

pygame.init()

app = GameApp()
app.run()

pygame.quit()
