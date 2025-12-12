#include <bitset>
#include <any>

enum class MtlTag
{
	Solid, Bulk, Liquid, Gas, Space, Float, N
};
constexpr auto MtlTagsCount = static_cast<size_t>(MtlTag::N);
using MaterialTags = std::bitset<MtlTagsCount>;

enum class MaterialPhysicalBehavior
{
	Null, Sand, Liquid, LightGas, HeavyGas
};

using MaterialID = uintptr_t;

struct DotProxy
{
	float &temp, &heat_capacity,
	      &thermal_conductivity;
	uint32_t &color;
	MaterialTags &tags;
	MaterialPhysicalBehavior &physical_behavior;
	std::any &aux;
	MaterialID &id;
};

