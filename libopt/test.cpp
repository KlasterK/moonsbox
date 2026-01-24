#include "gamemap.hpp"
#include "simulation.hpp"
#include "materials.hpp"
#include "drawing.hpp"
#include <iostream>
#include <SDL2/SDL.h>

int main()
{
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);

    GameMap game_map(100, 100);
    SimulationManager sim(game_map);

    Space space(game_map);
    sim.register_controller(space, "Space");
    drawing::fill(game_map, [&](size_t x, size_t y){ space.init_point(x, y); });

    Sand sand(game_map);
    sim.register_controller(sand, "Sand");
    drawing::rect(game_map, {10, 80, 10, 10}, [&](size_t x, size_t y){ sand.init_point(x, y); });

    auto *wnd = SDL_CreateWindow(
        "Здравствуйте и процветайте!",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        400, 400,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    auto *rnd = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    auto* tex = SDL_CreateTexture(rnd,
        game_map.colors.surface().format->format,  // Тот же формат что и у поверхности!
        SDL_TEXTUREACCESS_STREAMING,
        game_map.colors.surface().w, game_map.colors.surface().h
    );

    bool running = true;
    while(running)
    {
        SDL_Event ev;
        while(SDL_PollEvent(&ev))
        {
            if(ev.type == SDL_QUIT)
                running = false;
        }

        sim.tick();
        SDL_UpdateTexture(tex, NULL, game_map.colors.surface().pixels, 
                          game_map.colors.surface().pitch);

        SDL_SetRenderDrawColor(rnd, 0, 0x33, 0, 255);
        SDL_RenderClear(rnd);
        
        int win_w, win_h;
        SDL_GetWindowSize(wnd, &win_w, &win_h);
        
        // 9. Рассчитываем прямоугольник для заполнения всего окна
        // с сохранением пропорций изображения
        float surface_aspect = (float)game_map.colors.surface().w / game_map.colors.surface().h;
        float window_aspect = (float)win_w / win_h;
        
        SDL_Rect dst_rect;
        if (window_aspect > surface_aspect) {
            // Окно шире относительно высоты
            int render_w = win_h * surface_aspect;
            dst_rect.x = (win_w - render_w) / 2;
            dst_rect.y = 0;
            dst_rect.w = render_w;
            dst_rect.h = win_h;
        } else {
            // Окно уже относительно высоты
            int render_h = win_w / surface_aspect;
            dst_rect.x = 0;
            dst_rect.y = (win_h - render_h) / 2;
            dst_rect.w = win_w;
            dst_rect.h = render_h;
        }
        
        // 10. Рендерим текстуру
        SDL_RenderCopyEx(rnd, tex, NULL, &dst_rect, 0.0, NULL, SDL_FLIP_VERTICAL);
        
        
        // 12. Обновляем экран
        SDL_RenderPresent(rnd);
        
        // Небольшая пауза для снижения нагрузки на CPU
        // SDL_Delay(16); // ~60 FPS
    }
}
