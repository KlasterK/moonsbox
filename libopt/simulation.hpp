#ifndef MOOX_SIMULATION_HPP
#define MOOX_SIMULATION_HPP

#include "gamemap.hpp"
#include "materialcontroller.hpp"
#include <vector>
#include <unordered_map>

class SimulationManager
{
public:
    SimulationManager(GameMap &map);
    SimulationManager(SimulationManager &&) = default;
    SimulationManager(const SimulationManager &) = delete;
    
    bool register_controller(MaterialController &controller, std::string_view name);
    MaterialController *find_controller_by_name(std::string_view name);
    MaterialController *find_controller_by_id(MaterialID id);

    void tick();
    inline bool is_paused() const  { return m_is_paused; }
    inline void set_paused(bool v) { m_is_paused = v; }

private:
    GameMap &m_map;
    std::unordered_map<std::string_view, MaterialController *> m_controllers;
    bool m_is_paused = false;
};

#endif // MOOX_SIMULATION_HPP
