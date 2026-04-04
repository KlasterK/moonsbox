#ifndef MOOX_DYNAMITE_HPP
#define MOOX_DYNAMITE_HPP

#include "simulationengine/algorithms/fastprng.hpp"
#include <cstddef>
#include <cstring>
#include <simulationengine/materials/common.hpp>
#include <stdexcept>

class Dynamite : public MaterialController
{
private:
    struct ShotData 
    {
        alignas(1) int8_t dir_x{}, dir_y{}, time_to_live{}, already_updated_in_tick_num{};
    };
    static_assert(sizeof(ShotData) == 4);

    inline void init_point_as_shot(GameMap &map, size_t x, size_t y, int8_t direction_x, int8_t direction_y)
    {
        map.temps(x, y) = 2000.f;
        map.heat_capacities(x, y) = 0.8f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = 0xFF0000FF | ((fastprng::get_u32() & 0xFF) << 16);
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y) = ShotData{.dir_x=direction_x, .dir_y=direction_y,
                                  .time_to_live=int8_t(25u + fastprng::get_u8() % 40u),
                                  .already_updated_in_tick_num=m_current_tick_num};
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

public:
    inline void init_point(GameMap &map, size_t x, size_t y) override
    {
        map.temps(x, y) = 300.f;
        map.heat_capacities(x, y) = 0.8f;
        map.thermal_conductivities(x, y) = 0.3f;
        map.colors(x, y) = (x % 5 == 0 || y % 5 == 0) ? 0x660000FF : 0xFF0000FF;
        map.tags(x, y).reset().set(MtlTag::Solid);
        map.auxs(x, y).reset();
        map.physical_behaviors(x, y) = MaterialPhysicalBehavior::Null;
        map.material_ctls(x, y) = this;
    }

    inline void pre_dynamic_update(GameMap &) override
    {
        if(m_registry == nullptr)
            throw std::logic_error("Dynamite::pre_dynamic_update: instance not registered");

        if(m_space == nullptr)
        {
            m_space = m_registry->find_controller_by_name("Space");
            if(m_space == nullptr)
                throw std::logic_error("Dynamite::pre_dynamic_update: required material Space not registered");
        }

        ++m_current_tick_num;
    }

    inline void dynamic_update_point(GameMap &map, size_t x, size_t y) override
    {
        if(auto *aux = std::any_cast<ShotData>(&map.auxs(x, y)); aux != nullptr)
        {
            // Protection from being updated multiple times in one tick.
            // Because the updating is going increasing X and Y,
            // a shot with direction (1; 1) would get updated multiple times in a tick
            // and immediately die.
            if(m_current_tick_num == aux->already_updated_in_tick_num)
                return;
            aux->already_updated_in_tick_num = m_current_tick_num;

            if(--aux->time_to_live > 0)
            {
                if(fastprng::probability(1, 10))
                {
                    aux->dir_x = int(fastprng::get_u32() % 3) - 1;
                    aux->dir_y = int(fastprng::get_u32() % 3) - 1;
                }

                size_t dst_x = x + aux->dir_x, dst_y = y + aux->dir_y;
                if (
                    map.in_bounds(dst_x, dst_y) 
                    && !map.tags(dst_x, dst_y).test(MtlTag::Unbreakable)
                    && map.material_ctls(dst_x, dst_y) != this
                )
                    drawing::copy(map, x, y, dst_x, dst_y);
            }

            float temp = map.temps(x, y);
            m_space->init_point(map, x, y);
            map.temps(x, y) = temp;
            return;
        }

        if(map.temps(x, y) < 400.f)
            return;
        
        static auto moore_deltas = g_moore_deltas;
        fastprng::shuffle(moore_deltas.begin(), moore_deltas.end());

        for(auto [dx, dy] : moore_deltas)
        {
            for(int distance{1}; distance < 5; ++distance)
            {
                size_t shot_x = x + dx * distance, shot_y = y + dy * distance;
                if(!map.in_bounds(shot_x, shot_y) || map.tags(shot_x, shot_y).test(MtlTag::Unbreakable))
                    continue;
                init_point_as_shot(map, shot_x, shot_y, dx, dy);
            }
        }

        m_space->init_point(map, x, y);
        map.temps(x, y) = 2000.f;
    }

    inline void on_register(MaterialRegistry &registry) override
    {
        m_registry = &registry;
    }

    inline bool is_placeable_on(GameMap &map, size_t x, size_t y) override
    {
        return !map.tags(x, y).test(MtlTag::Unbreakable);
    }

    inline std::pair<std::vector<uint8_t>, SemanticVersion> 
        serialize(const GameMap &map, size_t x, size_t y) override
    {
        auto *shot_data = std::any_cast<ShotData>(&map.auxs(x, y));
        if(shot_data == nullptr)
            return {};

        std::vector<uint8_t> raw_data;
        raw_data.resize(sizeof(ShotData));
        std::memcpy(raw_data.data(), shot_data, sizeof(ShotData));

        return {
            std::move(raw_data),
            {2, 0, 0, 0}
        };
    }

    inline DeserializationResult deserialize(GameMap &map, size_t x, size_t y, 
                                            std::span<const uint8_t> data, 
                                            SemanticVersion) override
    { 
        if(data.size() != sizeof(ShotData))
            return DeserializationResult::InvalidDataLength;

        map.auxs(x, y) = *reinterpret_cast<const ShotData *>(data.data());
        return DeserializationResult::Success;
    }

private:
    PlaySoundCallback m_play_sound_cb{nullptr};
    MaterialRegistry *m_registry = nullptr;
    MaterialController *m_space = nullptr;
    int8_t m_current_tick_num = 0;
};


#endif // MOOX_DYNAMITE_HPP
