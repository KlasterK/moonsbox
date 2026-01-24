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
    bool register_controller(MaterialController &controller, std::string_view name);
    MaterialController *find_controller_by_name(std::string_view name);
    void tick();

private:
    GameMap &m_map;
    std::unordered_map<std::string_view, MaterialController *> m_controllers;
};

#endif // MOOX_SIMULATION_HPP
