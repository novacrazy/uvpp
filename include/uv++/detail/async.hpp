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
            template <typename Functor, typename K>
            static inline void dispatch( std::promise<R> &result, Functor f, K args ) {
                try {
                    result.set_value( invoke( f, args ));

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <>
        struct dispatch_helper<void> {
            template <typename Functor, typename K>
            static inline void dispatch( std::promise<void> &result, Functor f, K args ) {
                try {
                    invoke( f, args );

                    result.set_value();

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <typename Functor>
        struct AsyncContinuationBase : public Continuation<Functor> {
            typedef typename detail::function_traits<Functor>::result_type result_type;
            typedef typename detail::function_traits<Functor>::tuple_type  tuple_type;

            std::shared_ptr<std::promise<result_type>>       r;
            std::shared_ptr<std::shared_future<result_type>> s;

            inline AsyncContinuationBase( Functor f )
                : Continuation<Functor>( f ) {
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

        template <typename Functor, size_t arity = detail::function_traits<Functor>::arity>
        struct AsyncContinuation : public AsyncContinuationBase<Functor> {
            typedef typename AsyncContinuationBase<Functor>::tuple_type  tuple_type;
            typedef typename AsyncContinuationBase<Functor>::result_type result_type;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor>( f ) {
            }

            std::shared_ptr<tuple_type> p;

            template <typename T>
            inline void dispatch( T * ) {
                dispatch_helper<result_type>::dispatch( *this->r, this->f, std::move( *p ));

                this->cleanup();

                p.reset();
            }

            template <typename... Args>
            inline std::shared_future<result_type> init( Args &&... args ) {
                this->p = std::make_shared<tuple_type>( std::forward<Args>( args )... );

                return this->base_init();
            }
        };

        template <typename Functor>
        struct AsyncContinuation<Functor, 1> : public AsyncContinuationBase<Functor> {
            typedef typename AsyncContinuationBase<Functor>::result_type result_type;
            typedef typename AsyncContinuationBase<Functor>::tuple_type  tuple_type;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor>( f ) {
            }

            template <typename T>
            inline void dispatch( T *t ) {
                dispatch_helper<result_type>::dispatch( *this->r, this->f, tuple_type( t ));

                this->cleanup();
            }

            template <typename... Args>
            inline std::shared_future<result_type> init( Args... args ) {
                return this->base_init();
            }
        };
    }
}
#endif //UV_ASYNC_DETAIL_HPP
