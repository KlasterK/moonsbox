import sys

import pygame

try:
    from src.ui import Button, Container, HBoxLayout, Ruleset, Selector, Stylesheet, Subwindow
except ImportError:
    sys.path.insert(0, '.')
    from src.ui import Button, Container, HBoxLayout, Ruleset, Selector, Stylesheet, Subwindow


def main():
    pygame.init()
    pygame.display.init()

    pygame.display.set_mode((300, 300), pygame.RESIZABLE)
    screen = pygame.display.get_surface()

    master = Container(
        style=Stylesheet(
            Selector(
                class_name='Subwindow',
                ruleset=Ruleset(
                    bg_color='white',
                    border_width=2,
                ),
            ),
            Selector(
                class_name='Button',
                pseudo='hover',
                ruleset=Ruleset(
                    bg_color="#009900",
                ),
            ),
            Selector(
                class_name='Button',
                pseudo='pressed',
                ruleset=Ruleset(
                    bg_color="#00cc00",
                ),
            ),
        )
    )

    popup = Subwindow('Saving', master)
    popup.set_rect(x=100, y=100, w=200, h=100)

    layout = HBoxLayout(popup)
    popup.set_central_widget(layout)
    btn_load = Button('Load', parent=layout)
    btn_save = Button('Save', parent=layout)

    while True:
        for e in pygame.event.get():
            if e.type == pygame.QUIT:
                pygame.quit()
                return
            master.process_event(e)

        screen.fill('black')
        master.draw(screen)
        pygame.display.flip()


if __name__ == '__main__':
    main()
