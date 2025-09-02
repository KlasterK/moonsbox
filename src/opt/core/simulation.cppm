module;
export module simulation;

import util;
import gamemap;

export class SimulationManager : private Clock
{
private:
    bool m_temp_is_exchanging;
    GameMap& m_map;

public:
    SimulationManager(GameMap& map, bool temp_is_exchanging)
        : m_map(map)
        , m_temp_is_exchanging(temp_is_exchanging)
    {}

    
};