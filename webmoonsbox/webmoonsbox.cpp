#include <simulationengine/algorithms/drawing.hpp>
#include <simulationengine/core/gamemap.hpp>
#include <simulationengine/core/materialregistry.hpp>
#include <simulationengine/materials/allmaterials.hpp>
#include <simulationengine/simulation/simulationmanager.hpp>
#include <simulationengine/algorithms/fastprng.hpp>
#include <emscripten.h>
#include <string_view>
#include <optional>
#include <iostream>
#include <print>

struct GameApp
{
    AllMaterialsT<std::tuple> materials_tuple{};
    MaterialRegistry registry{};
    GameMap map;
    SimulationManager simulation_manager;

    GameApp(size_t width, size_t height)
        : map(width, height)
        , simulation_manager(map, registry)
    {
        register_all_materials(materials_tuple, registry);
        drawing::fill(map, drawing::make_controller_init_point_factory(
            map, std::get<Space>(materials_tuple)
        ));
    }
};

std::optional<GameApp> g_app_opt;

extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    const char *init_game(size_t width, size_t height)
    {
        g_app_opt.emplace(width, height);

        static std::string buffers_json{std::format(
            "{{\"colors_ptr\": {}}}",
            reinterpret_cast<uintptr_t>(g_app_opt->map.colors.span().data())
        )};
        return buffers_json.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
    bool tick(bool do_set_paused, bool value)
    {
        auto &app = g_app_opt.value();
        if(do_set_paused)
            app.simulation_manager.set_paused(value);
        app.simulation_manager.tick();
        return app.simulation_manager.is_paused();
    }

    EMSCRIPTEN_KEEPALIVE
    bool draw_dot(int x, int y, const char *material)
    {
        auto &app = g_app_opt.value();
        if(!app.map.in_bounds(x, y))
            return false;

        auto *p = app.registry.find_controller_by_name(material);
        if(!p)
            return false;
            
        p->init_point(app.map, x, y);
        return true;
    }

    EMSCRIPTEN_KEEPALIVE
    bool play_place_sound(int x, int y, const char *material)
    {
        auto &app = g_app_opt.value();
        auto *p = app.registry.find_controller_by_name(material);
        if(!p)
            return false;
            
        p->play_place_sound(app.map, x, y);
        return true;
    }

    EMSCRIPTEN_KEEPALIVE
    void register_play_sound_callback(int func_id)
    {
        if(func_id == 0)
            return;

        auto cb = [func_id](
            std::string_view name,
            std::optional<std::string_view> category,
            bool do_override
        )
        {
            reinterpret_cast<void(*)(const char *, const char *, int)>(func_id)(
                std::string(name).c_str(),
                category ? std::string(*category).c_str() : nullptr,
                do_override
            );
        };

        for(auto &pair : g_app_opt.value().registry)
            pair.second->set_play_sound_callback(cb);
    }
}
