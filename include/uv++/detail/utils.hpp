//
// Created by Aaron on 7/29/2016.
//

#ifndef UV_UTILS_DETAIL_HPP
#define UV_UTILS_DETAIL_HPP

#include <tuple>
#include <future>

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

        template <typename Functor, typename T, std::size_t... S>
        inline decltype( auto ) invoke_helper( Functor &&func, T &&t, std::index_sequence<S...> ) {
            return func( std::get<S>( std::forward<T>( t ))... );
        }

        template <typename Functor, typename T>
        inline decltype( auto ) invoke( Functor &&func, T &&t ) {
            constexpr auto Size = std::tuple_size<typename std::decay<T>::type>::value;

            return invoke_helper( std::forward<Functor>( func ),
                                  std::forward<T>( t ),
                                  std::make_index_sequence<Size>{} );
        }

        template <typename T>
        inline std::future<T> make_ready_future( T &&t ) {
            std::promise<T> p;
            p.set_value( t );
            return p.get_future();
        }

        template <typename T, typename E>
        inline std::future<T> make_exception_future( E e ) {
            std::promise<T> p;
            p.set_exception( std::make_exception_ptr( e ));
            return p.get_future();
        };

        template <typename T, bool = std::is_fundamental<T>::value>
        struct LazyStatic {
            typedef T value_type;

            value_type       *value;
            std::atomic_bool ran;

            virtual void init() = 0;

            inline operator value_type &() {
                return this->get();
            }

            inline value_type &get() {
                bool expect_ran = false;

                ran.compare_exchange_strong( expect_ran, true );

                if( !expect_ran ) {
                    this->init();
                }

                return *value;
            }

            ~LazyStatic() {
                delete this->value;
            }
        };

        /*
         * Specialization for fundemental type that don't require heap allocations
         * */
        template <typename T>
        struct LazyStatic<T, true> {
            typedef T value_type;

            value_type       value;
            std::atomic_bool ran;

            virtual void init() = 0;

            inline operator value_type &() {
                return this->get();
            }

            inline value_type &get() {
                bool expect_ran = false;

                ran.compare_exchange_strong( expect_ran, true );

                if( !expect_ran ) {
                    this->init();
                }

                return value;
            }
        };
    }
}

#endif //UV_UTILS_DETAIL_HPP
