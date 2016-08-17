//
// Created by Aaron on 8/17/2016.
//

#ifndef UV_THEN_DETAIL_HPP
#define UV_THEN_DETAIL_HPP

#include "../defines.hpp"
#include "function_traits.hpp"

#include <future>

/*
 * The `then` function implemented below is designed to be very similar to JavaScript's Promise.then in functionality.
 *
 * You provide it with a promise or future/shared_future object, and it will invoke a callback with the result of that promise or future.
 *
 * Additionally, it will resolve recursive futures. As in, if the original future resolves to another future, and so forth.
 *
 * It will also resolve futures/promises returned by the callback, so the future returned by `then` will always resolve to a non-future type.
 *
 * Basically, you can layer up whatever you want and it'll resolve them all.
 * */

namespace uv {
    namespace detail {
        using namespace std;

        /*
         * The default launch policy of std::async is a combination of the std::launch flags,
         * allowing it to choose whatever policy it wants depending on the system.
         * */
        constexpr launch default_policy = launch::deferred | launch::async;

        /*
         * This is a special launch type where it is guarenteed to spawn a new thread to run the task
         * asynchronously. It will not wait on any calls to .get() on the resulting future.
         *
         * 3 was chosen because deferred and async are usually 1 and 2, though it doesn't matter since this
         * is a type-safe enum class anyway.
         * */

        enum class uv_launch {
                detached = 3
        };

        //////////

        /*
         * Forward declarations and default arguments for standard then overloads
         * */

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( future<T> &, Functor, launch = default_policy );

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( shared_future<T>, Functor, launch = default_policy );

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( future<T> &&, Functor, launch = default_policy );

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( promise<T> &, Functor, launch = default_policy );

        //////////

        /*
         * Forward declarations (without default arguments) for detached then overloads.
         *
         * There are no default overloads because these will ONLY be called if the uv_launch::detached
         * flag is given, ensuring these overloads are called instead of the above.
         * */

        //TODO: Implement these

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( future<T> &, Functor, uv_launch );

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( shared_future<T>, Functor, uv_launch );

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( future<T> &&, Functor, uv_launch );

        template <typename T, typename Functor>
        UV_DECLTYPE_AUTO then( promise<T> &, Functor, uv_launch );

        //////////

        template <typename... Args>
        UV_DECLTYPE_AUTO then2( Args... );

        //////////

        /*
         * recursive_get:
         *
         * This structure will continually call "get" on any future object returned by another future,
         * recursively, basically resolving the entire chain at once
         * */

        /*
         * If K is not a future type, just return k.get()
         * */
        template <typename K>
        struct recursive_get {
            inline static UV_DECLTYPE_AUTO get( future<K> &&k ) {
                return k.get();
            }

            /*
             * Overload for shared_future, which can be passed by value if needed
             * */
            inline static UV_DECLTYPE_AUTO get( shared_future<K> k ) {
                return k.get();
            }

            /*
             * Overload for promises
             * */
            inline static UV_DECLTYPE_AUTO get( promise<K> &&k ) {
                return recursive_get<K>::get( k.get_future());
            }
        };

        /*
         * If K was a future type, call recursive_get again on the result of k.get()
         * */
        template <typename P>
        struct recursive_get<future<P>> {
            /*
             * Overload for a future resolving to a future
             * */
            inline static UV_DECLTYPE_AUTO get( future<future<P>> &&k ) {
                return recursive_get<P>::get( k.get());
            }

            /*
             * Overload for a shared_future resolving to a future
             * */
            inline static UV_DECLTYPE_AUTO get( shared_future<future<P>> k ) {
                return recursive_get<P>::get( k.get());
            }
        };

        /*
         * If K was a shared_future type, call recursive_get again on the result of k.get()
         * */
        template <typename P>
        struct recursive_get<shared_future<P>> {
            /*
             * Overload for future resolving to a shared_future
             * */
            inline static UV_DECLTYPE_AUTO get( future<shared_future<P>> &&k ) {
                return recursive_get<P>::get( k.get());
            }

            /*
             * Overload for shared_future resolving to another shared_future
             * */
            inline static UV_DECLTYPE_AUTO get( shared_future<shared_future<P>> k ) {
                return recursive_get<P>::get( k.get());
            }
        };

        //////////

        /*
         * optional_get
         *
         * This structure will recursively call get() only if the type is a future,
         * otherwise it just forwards the value through like it wasn't there
         * */

        /*
         * The is the raw version that just passes the value through like it wasn't there
         * */
        template <typename T>
        struct optional_get {
            /*
             * No special forwarding or moves are done by this, because the compiler should
             * optimize away any copies, given the sheer simplicity of this. I mean, it's basically the
             * epitome of the identity function, especially with the decltype(auto) allowing it to choose
             * the best return type for perfectly forwarding it.
             * */

            inline static UV_DECLTYPE_AUTO get( T t ) {
                return t;
            }
        };

        /*
         * Specialization for future
         * */
        template <typename R>
        struct optional_get<future<R>> {
            inline static UV_DECLTYPE_AUTO get( future<R> &&r ) {
                return recursive_get<R>::get( move( r ));
            }
        };

        /*
         * Specialization for shared_future
         * */
        template <typename R>
        struct optional_get<shared_future<R>> {
            inline static UV_DECLTYPE_AUTO get( shared_future<R> r ) {
                return recursive_get<R>::get( r );
            }
        };

        //////////

        /*
         * then_invoke_helper structure
         *
         * This just takes care of the invocation of the callback, forwarding arguments and so forth and
         * taking care of any returned futures
         * */

        template <typename Functor, typename R = fn_result_of <Functor>>
        struct then_invoke_helper {
            template <typename... Args>
            inline static UV_DECLTYPE_AUTO invoke( Functor f, Args... args ) {
                return optional_get<R>::get( f( forward<Args>( args )... ));
            }
        };

        /*
         * Specialization of then_invoke_helper for void callbacks.
         * */
        template <typename Functor>
        struct then_invoke_helper<Functor, void> {
            template <typename... Args>
            inline static void invoke( Functor f, Args... args ) {
                f( forward<Args>( args )... );
            }
        };

        //////////

        /*
         * then_helper structure
         *
         * This takes care of resolving the first given future, and optionally passing the
         * resulting value to the callback function.
         * */

        template <typename T, typename Functor>
        struct then_helper {
            inline static UV_DECLTYPE_AUTO dispatch( launch policy, future<T> &&s, Functor f ) {
                return then_invoke_helper<Functor>::invoke( f, recursive_get<T>::get( move( s )));
            }

            inline static UV_DECLTYPE_AUTO dispatch( launch policy, shared_future<T> s, Functor f ) {
                return then_invoke_helper<Functor>::invoke( f, recursive_get<T>::get( s ));
            }
        };

        template <typename Functor>
        struct then_helper<void, Functor> {
            inline static UV_DECLTYPE_AUTO dispatch( launch policy, future<void> &&s, Functor f ) {
                s.get();

                return then_invoke_helper<Functor>::invoke( f );
            }

            inline static UV_DECLTYPE_AUTO dispatch( launch policy, shared_future<void> s, Functor f ) {
                s.get();

                return then_invoke_helper<Functor>::invoke( f );
            }
        };

        //////////

        /*
         * then function
         *
         * This is the overload of then that resolves futures and promises and invokes a callback with the
         * resulting value. It uses the standard std::async for asynchronous invocation.
         * */

        /*
         * Overload for simple futures
         * */
        template <typename T, typename Functor>
        inline UV_DECLTYPE_AUTO then( future<T> &&s, Functor f, launch policy ) {
            return async( policy, [policy, f]( future <T> &&s2 ) {
                return then_helper<T, Functor>::dispatch( policy, move( s2 ), f );
            }, move( s ));
        };

        /*
         * Overload for futures by references. It will take ownership and move the future,
         * calling the above overload.
         * */
        template <typename T, typename Functor>
        inline UV_DECLTYPE_AUTO then( future<T> &s, Functor f, launch policy ) {
            return then( move( s ), f, policy );
        };

        /*
         * Overload for shared_futures, passed by value because they can be copied.
         * It's similar to the first overload, but can capture the shared_future in the lambda.
         * */
        template <typename T, typename Functor>
        inline UV_DECLTYPE_AUTO then( shared_future<T> s, Functor f, launch policy ) {
            return async( policy, [policy, s, f]() {
                return then_helper<T, Functor>::dispatch( policy, s, f );
            } );
        };

        /*
         * Overload for promise references. The promise is NOT moved, but rather the future is acquired
         * from it and the promise is left intact, so it can be used elsewhere.
         * */
        template <typename T, typename Functor>
        inline UV_DECLTYPE_AUTO then( promise<T> &s, Functor f, launch policy ) {
            return then( s.get_future(), f, policy );
        };

        //////////

        /*
         * Just a little trick to get the type of a future when it's not known beforehand.
         *
         * Used in then2 below.
         * */

        template <typename K>
        struct get_future_type {
            //No type is given here
        };

        template <typename T>
        struct get_future_type<future<T>> {
            typedef T type;
        };

        //////////

        /*
         * This is a future object functionally equivalent to std::future,
         * but with a .then function to chain together many futures.
         * */

        template <typename T>
        class ThenableFuture : public future<T> {
            public:
                constexpr ThenableFuture() noexcept : future<T>() {}

                inline ThenableFuture( future<T> &&f ) noexcept : future<T>( move( f )) {}

                inline ThenableFuture( ThenableFuture &&f ) noexcept : future<T>( move( f )) {}

                ThenableFuture( const ThenableFuture & ) = delete;

                ThenableFuture &operator=( const ThenableFuture & ) = delete;

                /*
                 * These operator overloads allow ThenableFuture to be used as a normal future rather easily
                 * */
                inline operator future<T> &() {
                    return *static_cast<future <T> *>(this);
                }

                inline operator future<T> &&() {
                    return move( *static_cast<future <T> *>(this));
                }

                template <typename... Args>
                inline UV_DECLTYPE_AUTO then( Args... args ) {
                    //NOTE: if `then` is used instead of `then2`, the namespace should be explicitely given, or this function will recurse
                    return then2( move( *this ), std::forward<Args>( args )... );
                }
        };

        //////////

        /*
         * then2 is a variation of then that returns a ThenableFuture instead of a normal future
         * */
        template <typename... Args>
        inline UV_DECLTYPE_AUTO then2( Args... args ) {
            auto &&k = then( std::forward<Args>( args )... );

            return ThenableFuture<typename get_future_type<typename remove_reference<decltype( k )>::type>::type>( move( k ));
        }
    }

    using detail::then;
    using detail::then2;
    using detail::uv_launch;
}

#endif //UV_THEN_DETAIL_HPP
