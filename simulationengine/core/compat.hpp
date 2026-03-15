#ifndef MOOX_COMPAT_HPP
#define MOOX_COMPAT_HPP

#include <functional>
#include <version>

template<typename... Ts>
#ifdef __cpp_lib_move_only_function
    using MoveOnlyOrOldFunction = std::move_only_function<Ts...>;
#else
    using MoveOnlyOrOldFunction = std::function<Ts...>;
#endif

#endif // MOOX_COMPAT_HPP
