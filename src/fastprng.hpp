#ifndef MOOX_FASTPRNG_HPP
#define MOOX_FASTPRNG_HPP

#include <cstdint>
#include <limits>

namespace fastprng
{
    extern uint64_t g_previous_value;
    
    inline uint64_t get_u64()
    {
        g_previous_value ^= g_previous_value << 13;
        g_previous_value ^= g_previous_value >> 7;
        g_previous_value ^= g_previous_value << 17;
        return g_previous_value;
    }

    inline uint32_t get_u32()
    {
        return static_cast<uint32_t>(get_u64());
    }

    inline uint8_t get_u8()
    {
        return static_cast<uint8_t>(get_u64());
    }

    inline float get_float()
    {
        return float(get_u64()) / float(std::numeric_limits<uint64_t>::max());
    }

    inline bool get_bool()
    {
        return get_u64() & 1;
    }

    inline bool propability(uint64_t numerator, uint64_t denominator)
    {
        return get_u64() < numerator * (std::numeric_limits<uint64_t>::max() / denominator);
    }
}

#endif // MOOX_FASTPRNG_HPP
