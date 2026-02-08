#ifndef MOOX_SIMULATION_HPP
#define MOOX_SIMULATION_HPP

#include "gamemap.hpp"
#include "materialregistry.hpp"

class SimulationManager
{
public:
    SimulationManager(GameMap &map, MaterialRegistry &registry);
    SimulationManager(SimulationManager &&) = default;
    SimulationManager(const SimulationManager &) = delete;

    void tick();
    inline bool is_paused() const  { return m_is_paused; }
    inline void set_paused(bool v) { m_is_paused = v; }

private:
    GameMap &m_map;
    MaterialRegistry &m_registry;
    bool m_is_paused = false;
};

#endif // MOOX_SIMULATION_HPP
