//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_TYPE_TRAITS_HPP
#define UV_TYPE_TRAITS_HPP

#include "defines.hpp"

#include <type_traits>

namespace uv {
    template <typename K, typename...>
    struct all_type;

    template <typename K>
    struct all_type<K> : std::true_type {
    };

    template <typename K, typename T, typename ...Rest>
    struct all_type<K, T, Rest...>
        : std::integral_constant<bool, std::is_convertible<T, K>::value && all_type<K, Rest...>::value> {
    };
}

#endif //UV_TYPE_TRAITS_HPP
