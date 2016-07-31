//
// Created by Aaron on 7/28/2016.
//

#ifndef UV_ASYNC_DETAIL_HPP
#define UV_ASYNC_DETAIL_HPP

#include "handle.hpp"
#include "type_traits.hpp"

#include <tuple>
#include <memory>
#include <future>

namespace uv {
    namespace detail {
        template <typename R>
        struct dispatch_helper {
            template <typename Functor, typename... Args>
            static inline void dispatch( std::promise<R> &result, Functor f, std::tuple<Args...> &&args ) {
                try {
                    result.set_value( invoke( f, args ));

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <>
        struct dispatch_helper<void> {
            template <typename Functor, typename... Args>
            static inline void dispatch( std::promise<void> &result, Functor f, std::tuple<Args...> &&args ) {
                try {
                    invoke( f, args );

                    result.set_value();

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <typename Functor, typename Self>
        struct AsyncContinuationBase : public Continuation<Functor, Self> {
            typedef typename detail::function_traits<Functor>::result_type result_type;
            typedef typename detail::function_traits<Functor>::tuple_type  tuple_type;

            std::shared_ptr<std::promise<result_type>>       r;
            std::shared_ptr<std::shared_future<result_type>> s;

            inline AsyncContinuationBase( Functor f )
                : Continuation<Functor, Self>( f ) {
            }

            inline std::shared_future<result_type> base_init() {
                if( !this->r ) {
                    this->r = std::make_shared<std::promise<result_type >>();
                }

                if( !this->s ) {
                    this->s = std::make_shared<std::shared_future<result_type >>( this->r->get_future());
                }

                return *this->s;
            }

            inline void cleanup() {
                this->r.reset();
                this->s.reset();
            }
        };

        template <typename Functor, typename Self, size_t arity = detail::function_traits<Functor>::arity>
        struct AsyncContinuation : public AsyncContinuationBase<Functor, Self> {
            typedef typename AsyncContinuationBase<Functor, Self>::tuple_type  tuple_type;
            typedef typename AsyncContinuationBase<Functor, Self>::result_type result_type;

            typedef ContinuationNeedsSelf <Functor, Self> needs_self;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor, Self>( f ) {
            }

            std::shared_ptr<tuple_type> p;

            template <typename T>
            inline void dispatch( T * ) {
                dispatch_helper<result_type>::dispatch( *this->r, this->f, std::move( *p ));

                this->cleanup();

                p.reset();
            }

            template <typename T, typename... Args>
            inline typename std::enable_if<
                std::is_same<T, Self>::value &&
                ContinuationNeedsSelf<Functor, T>::value, std::shared_future<result_type>>::type
            init( T *self, Args &&... args ) {
                this->p = std::make_shared<tuple_type>( self, std::forward<Args>( args )... );

                return this->base_init();
            }

            template <typename T, typename... Args>
            inline typename std::enable_if<
                std::is_same<T, Self>::value &&
                !ContinuationNeedsSelf<Functor, T>::value, std::shared_future<result_type>>::type
            init( T *self, Args &&... args ) {
                this->p = std::make_shared<tuple_type>( std::forward<Args>( args )... );

                return this->base_init();
            }
        };

        template <typename Functor, typename Self>
        struct AsyncContinuation<Functor, Self, 1> : public AsyncContinuationBase<Functor, Self> {
            typedef typename AsyncContinuationBase<Functor, Self>::result_type result_type;
            typedef typename AsyncContinuationBase<Functor, Self>::tuple_type  tuple_type;

            typedef ContinuationNeedsSelf <Functor, Self> needs_self;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor, Self>( f ) {
            }

            template <typename T>
            inline typename std::enable_if<
                std::is_same<T, Self>::value && ContinuationNeedsSelf<Functor, T>::value>::type
            dispatch( T *t ) {
                dispatch_helper<result_type>::dispatch( *this->r, this->f, tuple_type( t ));

                this->cleanup();
            }

            template <typename T>
            inline typename std::enable_if<
                std::is_same<T, Self>::value && !ContinuationNeedsSelf<Functor, T>::value>::type
            dispatch( T *t ) {
                dispatch_helper<result_type>::dispatch( *this->r, this->f, tuple_type());

                this->cleanup();
            }

            template <typename T, typename... Args>
            inline typename std::enable_if<std::is_same<T, Self>::value, std::shared_future<result_type>>::type
            init( T *self, Args... args ) {
                return this->base_init();
            }
        };

        template <typename Functor, typename Self>
        struct AsyncContinuation<Functor, Self, 0> : public AsyncContinuationBase<Functor, Self> {
            typedef typename AsyncContinuationBase<Functor, Self>::result_type result_type;
            typedef typename AsyncContinuationBase<Functor, Self>::tuple_type  tuple_type;

            typedef ContinuationNeedsSelf <Functor, Self> needs_self;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor, Self>( f ) {
            }

            template <typename T>
            inline void dispatch( T *t ) {
                dispatch_helper<result_type>::dispatch( *this->r, this->f, tuple_type());

                this->cleanup();
            }

            template <typename T, typename... Args>
            inline typename std::enable_if<std::is_same<T, Self>::value, std::shared_future<result_type>>::type
            init( T *self, Args... args ) {
                return this->base_init();
            }
        };
    }
}
#endif //UV_ASYNC_DETAIL_HPP
