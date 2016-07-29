//
// Created by Aaron on 7/29/2016.
//

#ifndef UV_UTILS_DETAIL_HPP
#define UV_UTILS_DETAIL_HPP

namespace uv {
    namespace detail {
        template <typename T, typename K>
        inline bool is_any( T val, K val2 ) {
            return val == val2;
        }

        template <typename T, typename K, typename... Args>
        inline bool is_any( T val, K val2, Args... args ) {
            return is_any( val, val2 ) || is_any( val, args... );
        }

        template <typename T>
        inline bool is_any( T ) {
            return false;
        }

        template <typename A, typename B>
        struct TrivialPair {
            typedef A first_type;
            typedef B second_type;

            first_type  first;
            second_type second;
        };

        template <typename T, typename K, K value>
        struct SetValueOnScopeExit {
            T &t;

            inline SetValueOnScopeExit( T &_t ) : t( _t ) {}

            inline ~SetValueOnScopeExit() {
                t = value;
            }
        };
    }
}

#endif //UV_UTILS_DETAIL_HPP
