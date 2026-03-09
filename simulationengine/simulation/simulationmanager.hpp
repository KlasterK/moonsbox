#ifndef MOOX_SIMULATIONMANAGER_HPP
#define MOOX_SIMULATIONMANAGER_HPP

class GameMap;
class MaterialRegistry;

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
    int m_tick_num{};
};

#endif // MOOX_SIMULATIONMANAGER_HPP
