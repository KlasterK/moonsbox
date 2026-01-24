#ifndef MOOX_SIMULATION_HPP
#define MOOX_SIMULATION_HPP

#include "gamemap.hpp"

class SimulationManager
{
public:
    SimulationManager(const GameMap &map);
    void tick();

private:
    const GameMap &m_map;
};

#endif // MOOX_SIMULATION_HPP
