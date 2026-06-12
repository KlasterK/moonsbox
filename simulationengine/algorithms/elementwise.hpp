#ifndef MOOX_ELEMENTWISE_HPP
#define MOOX_ELEMENTWISE_HPP

#include <array>
#include <cstddef>

namespace array_element_wise_operators
{
    template<std::integral T, std::integral U, size_t N>
    constexpr auto operator+(std::array<T, N> a, std::array<U, N> b)
    {
        std::array<std::common_type_t<T, U>, N> result;
        for(size_t i{}; i < N; ++i)
            result[i] = a[i] + b[i];
        return result;
    }

    template<std::integral T, std::integral U, size_t N>
    constexpr auto operator-(std::array<T, N> a, std::array<U, N> b)
    {
        std::array<std::common_type_t<T, U>, N> result;
        for(size_t i{}; i < N; ++i)
            result[i] = a[i] + b[i];
        return result;
    }
}

#endif // MOOX_ELEMENTWISE_HPP
