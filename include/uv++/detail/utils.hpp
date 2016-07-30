//
// Created by Aaron on 7/29/2016.
//

#ifndef UV_UTILS_DETAIL_HPP
#define UV_UTILS_DETAIL_HPP

namespace uv {
    namespace detail {
        /*
         * The is_any function is to compare a value to any number of other values, generating a long string of
         * (value == other1) || (value == other2) || ... etc
         * */

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

        /*
         * This exists because Boost lockfree structures require types that can be trivially copied,
         * with absolutely no overloaded operator= in them. So that rules out std::pair
         *
         * This also doesn't have a constructor because I can use TrivialPair{first_value, second_value}
         * semantics to initialize it. Note the curly braces instead of parenthesis.
         *
         * It's about as basic a pair structure as you can get, which is the idea. It's just going to be holding
         * pointers or other primitive types anyway.
         * */
        template <typename A, typename B>
        struct TrivialPair {
            typedef A first_type;
            typedef B second_type;

            first_type  first;
            second_type second;
        };
    }
}

#endif //UV_UTILS_DETAIL_HPP
