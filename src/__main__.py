import pygame

from .gameapp import GameApp


def main():
    pygame.init()
    app = GameApp()
    app.run()
    pygame.quit()


if __name__ == '__main__':
    main()
