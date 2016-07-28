//
// Created by Aaron on 7/28/2016.
//

#ifndef UV_ASYNC_HPP
#define UV_ASYNC_HPP

#include "handle.hpp"

#include <future>

namespace uv {
    namespace detail {
        template <typename K>
        struct dispatch_helper {
            template <typename Functor, typename... Args>
            static inline void dispatch( std::promise<K> &result, Functor f, Args... args ) {
                try {
                    result.set_value( f( std::forward<Args>( args )... ));

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <>
        struct dispatch_helper<void> {
            template <typename Functor, typename... Args>
            static inline void dispatch( std::promise<void> &result, Functor f, Args... args ) {
                try {
                    f( std::forward<Args>( args )... );

                    result.set_value();

                } catch( ... ) {
                    result.set_exception( std::current_exception());
                }
            }
        };

        template <typename Functor, typename P, typename R>
        struct AsyncContinuation : public Continuation<Functor> {
            typedef P parameter_type;
            typedef R return_type;

            inline AsyncContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }

            std::shared_ptr<std::promise<return_type>>       r;
            std::shared_ptr<std::shared_future<return_type>> s;
            std::shared_ptr<parameter_type>                  p;

            template <typename T>
            inline void dispatch( const T &t ) {
                dispatch_helper<return_type>::dispatch( *this->r, this->f, t, *p );

                this->p.reset();
            }

            template <typename... Args>
            std::shared_future<return_type> init( Args &&... args ) {
                if( !this->r ) {
                    this->r = std::make_shared<std::promise<return_type>>();
                }

                this->p = std::make_shared<parameter_type>( std::forward<Args>( args )... );

                if( !this->s ) {
                    this->s = std::make_shared<std::shared_future<return_type>>( this->r->get_future());
                }

                return *this->s;
            }
        };

        template <typename Functor, typename R>
        struct AsyncContinuation<Functor, void, R> : public Continuation<Functor> {
            typedef void parameter_type;
            typedef R    return_type;

            inline AsyncContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }

            std::shared_ptr<std::promise<return_type>>       r;
            std::shared_ptr<std::shared_future<return_type>> s;

            template <typename T>
            inline void dispatch( const T &t ) {
                dispatch_helper<return_type>::dispatch( *this->r, this->f, t );
            }

            std::shared_future<return_type> init() {
                if( !this->r ) {
                    this->r = std::make_shared<std::promise<return_type>>();
                }

                if( !this->s ) {
                    this->s = std::make_shared<std::shared_future<return_type>>( this->r->get_future());
                }

                return *this->s;
            }
        };
    }

    template <typename P = void, typename R = void>
    class Async final : public Handle<uv_async_t> {
        public:
            typedef typename Handle<uv_async_t>::handle_t handle_t;

        protected:
            void _init() {
                //No-op for uv_async_t
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::AsyncContinuation<Functor, P, R> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_async_init( this->_loop, this->handle(), []( uv_async_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Cont *c = static_cast<Cont *>(d->continuation.get());

                    c->dispatch( *static_cast<Async<P, R> *>(d->self));
                } );
            }

            template <typename... Args>
            inline std::shared_future<R> send( Args &&... args ) {
                typedef detail::AsyncContinuation<void *, P, R> Cont;

                Cont *c = static_cast<Cont *>(this->internal_data.continuation.get());

                auto ret = c->init( std::forward<Args>( args )... );

                uv_async_send( this->handle());

                return ret;
            }

            inline void stop() {
                this->stop( []( auto ) {} );
            }

            template <typename Functor2>
            void stop( Functor2 f ) {
                this->internal_data.secondary_continuation = std::make_shared<Continuation<Functor2 >>( f );

                this->close( []( uv_handle_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Async *self = static_cast<Async *>(d->self);

                    static_cast<Continuation <Functor2> *>(d->secondary_continuation.get())->f( *self );

                    d->secondary_continuation.reset();
                } );
            }
    };
}

#endif //UV_ASYNC_HPP
