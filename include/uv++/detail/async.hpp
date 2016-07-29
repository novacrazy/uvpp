//
// Created by Aaron on 7/28/2016.
//

#ifndef UV_ASYNC_DETAIL_HPP
#define UV_ASYNC_DETAIL_HPP

#include "handle.hpp"

#include <memory>
#include <future>
#include <atomic>

namespace uv {
    namespace detail {
        enum {
            ASYNC_STATUS_INCOMPLETE,
            ASYNC_STATUS_COMPLETED,
            ASYNC_STATUS_THREW
        };

        template <typename K>
        struct dispatch_helper {
            template <typename Functor, typename T, typename... Args>
            static inline void
            dispatch( std::promise<K> &result, Functor f, std::atomic_int &status, T *t, Args... args ) {
                try {
                    auto &&r = f( *t, std::forward<Args>( args )... );

                    status = ASYNC_STATUS_COMPLETED;

                    result.set_value( r );

                } catch( ... ) {
                    status = ASYNC_STATUS_THREW;

                    result.set_exception( std::current_exception());
                }
            }
        };

        template <>
        struct dispatch_helper<void> {
            template <typename Functor, typename T, typename... Args>
            static inline void
            dispatch( std::promise<void> &result, Functor f, std::atomic_int &status, T *t, Args... args ) {
                try {
                    f( *t, std::forward<Args>( args )... );

                    status = ASYNC_STATUS_COMPLETED;

                    result.set_value();

                } catch( ... ) {
                    status = ASYNC_STATUS_THREW;

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
            std::atomic_int                                  status;

            inline AsyncContinuationBase( Functor f )
                : Continuation<Functor>( f ), status( ASYNC_STATUS_INCOMPLETE ) {
            }

            inline std::shared_future<return_type> base_init( std::atomic_bool &should_send ) {
                //Atomically test if the last dispatch threw and replace it with ASYNC_STATUS_INCOMPLETE for the next dispatch
                if( this->status.exchange( ASYNC_STATUS_INCOMPLETE ) == ASYNC_STATUS_THREW ) {
                    std::shared_future<return_type> old_s = *this->s;

                    this->r.reset();
                    this->s.reset();

                    should_send = false;

                    return old_s;

                } else {
                    this->r = std::make_shared<std::promise<return_type >>();
                    this->s = std::make_shared<std::shared_future<return_type >>( this->r->get_future());

                    return *this->s;
                }
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
                dispatch_helper<return_type>::dispatch( *this->r, this->f, this->status, t, *p );

                /*
                 * After dispatch, the status flag will be ASYNC_STATUS_COMPLETED, unless there was a another call to
                 * send in the time between the callback returning and the dispatch function returning.
                 *
                 * I should point out this can happen, and it's a very annoying data race.
                 * */

                if( this->status == ASYNC_STATUS_COMPLETED ) {
                    p.reset();
                }
            }

            template <typename... Args>
            inline std::shared_future<return_type> init( std::atomic_bool &should_send, Args &&... args ) {
                this->p = std::make_shared<parameter_type>( std::forward<Args>( args )... );

                return this->base_init( should_send );
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
                dispatch_helper<return_type>::dispatch( *this->r, this->f, this->status, t );
            }

            template <typename... Args>
            inline std::shared_future<return_type> init( std::atomic_bool &should_send ) {
                return this->base_init( should_send );
            }
        };
    }
}
#endif //UV_ASYNC_DETAIL_HPP
