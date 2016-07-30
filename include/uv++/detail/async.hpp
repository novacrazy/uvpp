//
// Created by Aaron on 7/28/2016.
//

#ifndef UV_ASYNC_DETAIL_HPP
#define UV_ASYNC_DETAIL_HPP

#include "handle.hpp"

#include <memory>
#include <future>

namespace uv {
    namespace detail {
        template <typename K>
        struct dispatch_helper {
            template <typename Functor, typename T, typename... Args>
            static inline void dispatch( std::promise<K> &result, Functor f, T *t, Args... args ) {
                try {
                    result.set_value( f( *t, std::forward<Args>( args )... ));

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <>
        struct dispatch_helper<void> {
            template <typename Functor, typename T, typename... Args>
            static inline void dispatch( std::promise<void> &result, Functor f, T *t, Args... args ) {
                try {
                    f( *t, std::forward<Args>( args )... );

                    result.set_value();

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <typename Functor, typename P, typename R>
        struct AsyncContinuationBase : public Continuation<Functor> {
            typedef P parameter_type;
            typedef R return_type;

            std::shared_ptr<std::promise<return_type>>       r;
            std::shared_ptr<std::shared_future<return_type>> s;
            std::mutex                                       m;

            inline AsyncContinuationBase( Functor f )
                : Continuation<Functor>( f ) {
            }

            inline std::shared_future<return_type> base_init() {
                this->r = std::make_shared<std::promise<return_type >>();
                this->s = std::make_shared<std::shared_future<return_type >>( this->r->get_future());

                return *this->s;
            }
        };

        template <typename Functor, typename P, typename R>
        struct AsyncContinuation : public AsyncContinuationBase<Functor, P, R> {
            typedef typename AsyncContinuationBase<Functor, P, R>::parameter_type parameter_type;
            typedef typename AsyncContinuationBase<Functor, P, R>::return_type    return_type;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor, P, R>( f ) {
            }

            std::shared_ptr<parameter_type> p;

            template <typename T>
            inline void dispatch( T *t ) {
                std::lock_guard<std::mutex> lock( this->m );

                dispatch_helper<return_type>::dispatch( *this->r, this->f, t, *p );

                p.reset();
            }

            template <typename... Args>
            inline std::shared_future<return_type> init( Args &&... args ) {
                std::lock_guard<std::mutex> lock( this->m );

                this->p = std::make_shared<parameter_type>( std::forward<Args>( args )... );

                return this->base_init();
            }
        };

        template <typename Functor, typename R>
        struct AsyncContinuation<Functor, void, R> : public AsyncContinuationBase<Functor, void, R> {
            typedef typename AsyncContinuationBase<Functor, void, R>::parameter_type parameter_type;
            typedef typename AsyncContinuationBase<Functor, void, R>::return_type    return_type;

            inline AsyncContinuation( Functor f )
                : AsyncContinuationBase<Functor, void, R>( f ) {
            }

            template <typename T>
            inline void dispatch( T *t ) {
                std::lock_guard<std::mutex> lock( this->m );

                dispatch_helper<return_type>::dispatch( *this->r, this->f, t );
            }

            template <typename... Args>
            inline std::shared_future<return_type> init() {
                std::lock_guard<std::mutex> lock( this->m );

                return this->base_init();
            }
        };
    }
}
#endif //UV_ASYNC_DETAIL_HPP
