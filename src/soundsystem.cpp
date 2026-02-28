#include "soundsystem.hpp"
#include <cassert>
#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "ext/miniaudio.h"

struct MaSoundDeleter
{
    void operator()(ma_sound *p) const noexcept
    {
        if (!p)
            return;
        ma_sound_uninit(p);
        delete p;
    }
};

using ma_sound_unique_ptr = std::unique_ptr<ma_sound, MaSoundDeleter>;

static bool g_was_inited{};
static ma_engine g_engine{};

static const auto ASSETS_SOUNDS_ROOT = std::filesystem::path("assets") / "sounds";

// TODO: don't reload a ma_sound each time it ends
static std::unordered_map<std::string, std::vector<std::filesystem::path>> g_tracks_cache;
// TODO: don't to an excess allocation here
static std::unordered_map<std::string, ma_sound_unique_ptr> g_current_sounds;
static std::unordered_map<std::string, ma_sound_unique_ptr> g_current_categories;

static std::mt19937_64 g_rng((std::random_device())());

struct SoundSystemCleanup
{
    ~SoundSystemCleanup()
    {
        if (!g_was_inited)
            return;
        // destroy any active sounds first (unique_ptr deleters will uninit)
        g_current_sounds.clear();
        g_current_categories.clear();
        ma_engine_uninit(&g_engine);
    }
} static g_cleanup;



void sfx::init()
{
    assert(!g_was_inited);
    g_was_inited = true;
    if (0 > ma_engine_init(nullptr, &g_engine))
        throw std::runtime_error("sfx::init: failed to init miniaudio engine");
}



// helper: load tracks for a sound name if not cached
static void _load_tracks(const std::string &sname) 
{
    if (g_tracks_cache.find(sname) != g_tracks_cache.end())
        return;

    std::vector<std::filesystem::path> found;
    if (
        std::filesystem::exists(ASSETS_SOUNDS_ROOT) 
        && std::filesystem::is_directory(ASSETS_SOUNDS_ROOT)
    )
    {
        for (auto &entry : std::filesystem::directory_iterator(ASSETS_SOUNDS_ROOT))
        {
            if (!entry.is_regular_file())
                continue;
            auto fn = entry.path().filename().string();
            // match prefix: name + '.'
            const std::string prefix = sname + '.';
            if (fn.size() > prefix.size() && fn.rfind(prefix, 0) == 0)
            {
                found.push_back(entry.path());
            }
        }
    }
    // store (may be empty)
    g_tracks_cache.emplace(sname, std::move(found));
};

void sfx::play_sound(
    std::string_view name, std::optional<std::string_view> category, bool do_override
)
{
    assert(g_was_inited);

    const std::string sound_name{name};
    const std::string key = category ? std::string(*category) : sound_name;

    auto &mapping = category ? g_current_categories : g_current_sounds;

    // check existing playing sound for this key
    if (auto it = mapping.find(key); it != mapping.end() && it->second)
    {
        ma_sound *cur = it->second.get();
        if (do_override)
        {
            ma_sound_stop(cur);
            mapping.erase(it);
        }
        else
        {
            if (ma_sound_is_playing(cur))
            {
                return; // already playing, do nothing
            }
            else
            {
                // finished or stopped, clear entry and allow new sound
                mapping.erase(it);
            }
        }
    }

    // ensure tracks are loaded
    _load_tracks(sound_name);
    const auto &tracks = g_tracks_cache[sound_name];
    if (tracks.empty())
        throw std::runtime_error("sfx::play_sound: no tracks found for sound '" + sound_name + "'");

    // pick a random track
    std::uniform_int_distribution<size_t> dist(0, tracks.size() - 1);
    const auto &chosen = tracks[dist(g_rng)];

    // create and start ma_sound
    auto *raw_sound_ptr = new ma_sound{};
    if (0 > ma_sound_init_from_file(
        &g_engine,
        chosen.string().c_str(),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        raw_sound_ptr
    ))
    {
        delete raw_sound_ptr;
        throw std::runtime_error(
            "sfx::play_sound: failed to init miniaudio sound for file '" 
            + chosen.string() + "'"
        );
    }

    ma_sound_unique_ptr sound{raw_sound_ptr};

    if (ma_sound_start(sound.get()) != MA_SUCCESS)
        throw std::runtime_error(
            "sfx::play_sound: failed to start miniaudio sound for file '" 
            + chosen.string() + "'"
        );

    mapping.emplace(key, std::move(sound));
}
