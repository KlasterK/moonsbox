#include <simulationengine/algorithms/fastprng.hpp>
#include <random>

uint64_t fastprng::g_previous_value = []
{
    std::random_device dev;
    std::uniform_int_distribution<uint64_t> dist;
    return dist(dev);
}();
