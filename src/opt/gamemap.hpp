#include <vector>

class GameMap
{
public:
	GameMap(size_t width, size_t height);

private:
	std::vector<float> m_temps,
		m_heat_capacities,
		m_thermal_conductivities;
	std::vector<uint32_t> m_colors;
	std::vector<MaterialTags> m_tags;
	std::vector<MaterialPhysicalBehavior>
		m_physical_behaviors;
	std::vector<std::any> m_auxs;

};

