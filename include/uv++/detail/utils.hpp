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

        inline std::future<void> make_ready_future( void ) {
            std::promise<void> p;
            p.set_value();
            return p.get_future();
        }

        template <typename T, typename E>
        inline std::future<T> make_exception_future( E e ) {
            std::promise<T> p;
            p.set_exception( std::make_exception_ptr( e ));
            return p.get_future();
        };

        template <typename T>
        struct LazyStatic {
            typedef T value_type;

            virtual value_type init() = 0;

            value_type               value;
            std::once_flag           once;
            std::atomic_bool         ran;
            std::promise<void>       p;
            std::shared_future<void> s;

            LazyStatic()
                : p(), s( p.get_future()) {
            }

            void run() {
                this->value = this->init();

                p.set_value();

                this->ran = true;
            }

            inline operator value_type &() {
                return this->get();
            }

            inline value_type &get() {
                if( !this->ran ) {
                    std::call_once( once, &LazyStatic::run, this );

                    if( !this->ran ) {
                        s.wait();
                    }
                }

                return value;
            }
        };

        template <class T, class Compare>
        inline const T &clamp( const T &v, const T &lo, const T &hi, Compare comp ) {
            assert( !comp( hi, lo ));

            return comp( v, lo ) ? lo : comp( hi, v ) ? hi : v;
        }

        template <class T>
        inline const T &clamp( const T &v, const T &lo, const T &hi ) {
            return clamp( v, lo, hi, std::less<>());
        }

        template <typename T, typename Functor>
        struct then_helper {
            template <typename... Args>
            inline static decltype( auto ) do_then( std::future<T> &&t, Functor &&f, Args... args ) {
                return f( t.get(), std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline static decltype( auto ) do_then( std::shared_future<T> &&t, Functor &&f, Args... args ) {
                return f( t.get(), std::forward<Args>( args )... );
            }
        };

        template <typename Functor>
        struct then_helper<void, Functor> {
            template <typename... Args>
            inline static decltype( auto ) do_then( std::future<void> &&t, Functor &&f, Args... args ) {
                t.get();

                return f( std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline static decltype( auto ) do_then( std::shared_future<void> &&t, Functor &&f, Args... args ) {
                t.get();

                return f( std::forward<Args>( args )... );
            }
        };

        template <typename T, typename Functor, typename... Args>
        inline decltype( auto ) then( std::launch policy, std::future<T> &&t, Functor &&f, Args... args ) {
            return std::async( policy, []( std::future<T> &&t2, Functor &&f2, Args... inner_args ) {
                return then_helper<T, Functor>::do_then( std::move( t2 ), std::move( f2 ),
                                                         std::forward<Args>( inner_args )... );
            }, std::move( t ), std::move( f ), std::forward<Args>( args )... );
        };

        template <typename T, typename Functor, typename... Args>
        inline decltype( auto ) then( std::future<T> &&t, Functor &&f, Args ... args ) {
            constexpr auto policy = std::launch::deferred | std::launch::async; //Default policy for std::async

            return std::async( policy, []( std::future<T> &&t2, Functor &&f2, Args... inner_args ) {
                return then_helper<T, Functor>::do_then( std::move( t2 ), std::move( f2 ),
                                                         std::forward<Args>( inner_args )... );
            }, std::move( t ), std::move( f ), std::forward<Args>( args )... );
        };

        template <typename T, typename Functor, typename... Args>
        inline decltype( auto ) then( std::launch policy, std::shared_future<T> &&t, Functor &&f, Args... args ) {
            return std::async( policy, []( std::shared_future<T> &&t2, Functor &&f2, Args... inner_args ) {
                return then_helper<T, Functor>::do_then( std::move( t2 ), std::move( f2 ),
                                                         std::forward<Args>( inner_args )... );
            }, std::move( t ), std::move( f ), std::forward<Args>( args )... );
        };

        template <typename T, typename Functor, typename... Args>
        inline decltype( auto ) then( std::shared_future<T> &&t, Functor &&f, Args ... args ) {
            constexpr auto policy = std::launch::deferred | std::launch::async; //Default policy for std::async

            return std::async( policy, []( std::shared_future<T> &&t2, Functor &&f2, Args... inner_args ) {
                return then_helper<T, Functor>::do_then( std::move( t2 ), std::move( f2 ),
                                                         std::forward<Args>( inner_args )... );
            }, std::move( t ), std::move( f ), std::forward<Args>( args )... );
        };

        template <typename T, typename... Args>
        inline decltype( auto ) then( std::promise<T> &t, Args... args ) {
            return then( t.get_future(), std::forward<Args>( args )... );
        };

        template <typename T, typename... Args>
        inline decltype( auto ) then( std::launch policy, std::promise<T> &t, Args... args ) {
            return then( policy, t.get_future(), std::forward<Args>( args )... );
        };
    }
}

#endif //UV_UTILS_DETAIL_HPP
