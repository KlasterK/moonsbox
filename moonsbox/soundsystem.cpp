#include "soundsystem.hpp"
#include <cassert>
#include <filesystem>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <list>
#include <simulationengine/algorithms/fastprng.hpp>
#include "ext/miniaudio.h"


struct MaSoundWrapper
{
    ma_sound sound{};

    MaSoundWrapper() = default;
    MaSoundWrapper(const MaSoundWrapper &) = delete;
    MaSoundWrapper(MaSoundWrapper &&) = delete;
    MaSoundWrapper &operator=(const MaSoundWrapper &) = delete;
    MaSoundWrapper &operator=(MaSoundWrapper &&) = delete;
    ~MaSoundWrapper() { ma_sound_stop(&sound); ma_sound_uninit(&sound); }
};

struct MaEngineWrapper
{
    ma_engine engine{};

    MaEngineWrapper() = default;
    MaEngineWrapper(const MaEngineWrapper &) = delete;
    MaEngineWrapper(MaEngineWrapper &&) = delete;
    MaEngineWrapper &operator=(const MaEngineWrapper &) = delete;
    MaEngineWrapper &operator=(MaEngineWrapper &&) = delete;
    ~MaEngineWrapper() { ma_engine_uninit(&engine); }
};

static std::optional<MaEngineWrapper> g_engine{};
static std::unordered_map<std::string, std::list<MaSoundWrapper>> g_tracks_cache;
static std::unordered_map<std::string, ma_sound *> g_current_sounds;
static std::unordered_map<std::string, ma_sound *> g_current_categories;
static const auto TracksRoot = std::filesystem::path("assets") / "sounds";


void sfx::init()
{
    if(g_engine.has_value())
        throw std::logic_error("sfx::init: sound system already inited");

    g_engine.emplace();
    if (0 > ma_engine_init(nullptr, &g_engine->engine))
    {
        g_engine.reset();
        throw std::runtime_error("sfx::init: failed to init miniaudio engine");
    }
}


// helper: load tracks for a sound name if not cached
static void _load_tracks(const std::string &sound_name) 
{
    auto [cache_it, was_inserted] = g_tracks_cache.try_emplace(sound_name);
    if(!was_inserted)
        return;

    auto &wrappers_list = cache_it->second;

    if (
        std::filesystem::exists(TracksRoot) 
        && std::filesystem::is_directory(TracksRoot)
    )
    {
        for (auto &entry : std::filesystem::directory_iterator(TracksRoot))
        {
            if (!entry.is_regular_file())
                continue;

            auto file_name = entry.path().filename().string();
            if(!file_name.starts_with(sound_name + "."))
                continue;
            
            if (0 > ma_sound_init_from_file(
                &g_engine->engine,
                entry.path().string().c_str(),
                MA_SOUND_FLAG_STREAM,
                nullptr,
                nullptr,
                &wrappers_list.emplace_back().sound
            ))
                throw std::runtime_error(std::format(
                    "sfx::play_sound: failed to init miniaudio sound for file '{}'", file_name
                ));
        }
    }
}


void sfx::play_sound(
    std::string_view name, std::optional<std::string_view> category, bool do_override
)
{
    if(!g_engine.has_value())
        throw std::logic_error("sfx::play_sound: sound system was not inited");

    const std::string sound_name{name};
    const std::string key = category ? std::string(*category) : sound_name;

    auto &mapping = category ? g_current_categories : g_current_sounds;

    // check existing playing sound for this key
    if (auto it = mapping.find(key); it != mapping.end() && it->second)
    {
        ma_sound *cur = it->second;
        if (do_override)
        {
            if (MA_SUCCESS != ma_sound_stop(it->second))
                throw std::runtime_error(std::format(
                    "sfx::play_sound: failed to stop miniaudio sound of '{}'", name
                ));

            if (MA_SUCCESS != ma_sound_seek_to_pcm_frame(it->second, 0))
                throw std::runtime_error(std::format(
                    "sfx::play_sound: failed to seek to PCM frame miniaudio sound of '{}'", name
                ));
        }
        else if(ma_sound_is_playing(cur))
            return; // already playing, do nothing
    }

    // ensure tracks are loaded
    _load_tracks(sound_name);
    auto &tracks = g_tracks_cache[sound_name];
    if (tracks.empty())
        throw std::runtime_error(std::format(
            "sfx::play_sound: no tracks found for sound '{}'", name
        ));

    // pick a random track
    auto chosen_track_it = tracks.begin();
    std::advance(chosen_track_it, fastprng::get_u64() % tracks.size());

    // start ma_sound of this track
    if (ma_sound_start(&chosen_track_it->sound) != MA_SUCCESS)
        throw std::runtime_error(std::format(
            "sfx::play_sound: failed to start miniaudio sound of '{}'", name
        ));
    
    mapping.insert_or_assign(key, &chosen_track_it->sound);
}
