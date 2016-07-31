//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_TYPE_TRAITS_DETAIL_HPP
#define UV_TYPE_TRAITS_DETAIL_HPP

#include <type_traits>
#include "function_traits.hpp"

namespace uv {
    namespace detail {
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
}

#endif //UV_TYPE_TRAITS_DETAIL_HPP
