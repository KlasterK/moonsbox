#ifndef MOOX_FASTPRNG_HPP
#define MOOX_FASTPRNG_HPP

#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <ranges>
#include <utility>

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

    inline bool probability(uint64_t numerator, uint64_t denominator)
    {
        return get_u64() < numerator * (std::numeric_limits<uint64_t>::max() / denominator);
    }

    template<std::ranges::random_access_range Range>
    inline void shuffle(Range &range)
    {
        auto begin = std::ranges::begin(range), end = std::ranges::end(range);
        if(begin == end)
            return;

        for(auto i = end - 1; i > begin; --i)
        {
            auto j = begin + get_u64() % (i - begin + 1);
            using std::swap;
            swap(*i, *j);
        }
    }

    template<std::ranges::forward_range Range>
    inline std::ranges::iterator_t<Range> choice(Range &&range)
    {
        auto size = std::ranges::size(range);
        if (size == 0)
            return std::ranges::end(range);
        
        auto it = std::ranges::begin(range);
        std::ranges::advance(it, get_u64() % size);
        return it;
    }

    template<typename T, typename... Args>
    inline T choice_va(Args &&...args)
    {
        return *choice(std::initializer_list<T>{std::forward<Args>(args)...});
    }
}

#endif // MOOX_FASTPRNG_HPP
