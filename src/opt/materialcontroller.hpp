#include "materialdefs.hpp"

class MaterialController
{
public:
	virtual void init_point(size_t x, size_t y) = 0;
	virtual void static_update() = 0;
	virtual void dynamic_update() = 0;

	inline MaterialID material_id()
	{
		return static_cast<MaterialID>(this);
	}
};

