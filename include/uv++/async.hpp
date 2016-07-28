//
// Created by Aaron on 7/28/2016.
//

#ifndef UV_ASYNC_HPP
#define UV_ASYNC_HPP

#include "handle.hpp"

#include <future>

namespace uv {
    namespace detail {
        template <typename Functor, typename P, typename R>
        struct AsyncContinuation : public Continuation<Functor> {
            typedef P parameter_type;
            typedef R return_type;

            AsyncContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }

            std::shared_ptr<std::promise<return_type>> r;
            std::shared_ptr<parameter_type>            p;
        };

        template <typename Functor, typename P>
        struct AsyncContinuation<Functor, P, void> : public Continuation<Functor> {
            typedef P    parameter_type;
            typedef void return_type;

            AsyncContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }

            std::shared_ptr<parameter_type> p;
        };

        template <typename Functor, typename R>
        struct AsyncContinuation<Functor, void, R> : public Continuation<Functor> {
            typedef void parameter_type;
            typedef R    return_type;

            AsyncContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }

            std::shared_ptr<std::promise<return_type>> r;
        };

        template <typename Functor>
        struct AsyncContinuation<Functor, void, void> : public Continuation<Functor> {
            typedef void parameter_type;
            typedef void return_type;

            AsyncContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }
        };

        template <typename P, typename R, typename Functor>
        typename std::enable_if<std::is_void<P>::value && std::is_void<R>::value>::type
        start_async( uv_loop_t *loop, uv_async_t *handle, HandleData *hd, Functor f ) {
            typedef AsyncContinuation<Functor, P, R> Cont;

            hd->continuation = std::make_shared<Cont>( f );

            uv_async_init( loop, handle, []( uv_async_t *h ) {
                HandleData *d = static_cast<HandleData *>(h->data);

                Cont *c = static_cast<Cont *>(d->continuation.get());

                auto self = static_cast<Async<P, R> *>(d->self);

                c->f( *self );
            } );
        };

        template <typename P, typename R, typename Functor>
        typename std::enable_if<std::is_void<P>::value && !std::is_void<R>::value>::type
        start_async( uv_loop_t *loop, uv_async_t *handle, HandleData *hd, Functor f ) {
            typedef AsyncContinuation<Functor, P, R> Cont;

            hd->continuation = std::make_shared<Cont>( f );

            uv_async_init( loop, handle, []( uv_async_t *h ) {
                HandleData *d = static_cast<HandleData *>(h->data);

                Cont *c = static_cast<Cont *>(d->continuation.get());

                auto self = static_cast<Async<P, R> *>(d->self);

                try {
                    c->r->set_value( c->f( *self ));

                } catch( ... ) {
                    c->r->set_exception( std::current_exception());
                }

                c->p.reset();
            } );
        };

        template <typename P, typename R, typename Functor>
        typename std::enable_if<!std::is_void<P>::value && std::is_void<R>::value>::type
        start_async( uv_loop_t *loop, uv_async_t *handle, HandleData *hd, Functor f ) {
            typedef AsyncContinuation<Functor, P, R> Cont;

            hd->continuation = std::make_shared<Cont>( f );

            uv_async_init( loop, handle, []( uv_async_t *h ) {
                HandleData *d = static_cast<HandleData *>(h->data);

                Cont *c = static_cast<Cont *>(d->continuation.get());

                auto self = static_cast<Async<P, R> *>(d->self);

                c->f( *self, *c->p );

                c->p.reset();
            } );
        };

        template <typename P, typename R, typename Functor>
        typename std::enable_if<!std::is_void<P>::value && !std::is_void<R>::value>::type
        start_async( uv_loop_t *loop, uv_async_t *handle, HandleData *hd, Functor f ) {
            typedef AsyncContinuation<Functor, P, R> Cont;

            hd->continuation = std::make_shared<Cont>( f );

            uv_async_init( loop, handle, []( uv_async_t *h ) {
                HandleData *d = static_cast<HandleData *>(h->data);

                Cont *c = static_cast<Cont *>(d->continuation.get());

                auto self = static_cast<Async<P, R> *>(d->self);

                try {
                    c->r->set_value( c->f( *self, *c->p ));

                } catch( ... ) {
                    c->r->set_exception( std::current_exception());
                }

                c->p.reset();
            } );
        };

        template <typename P, typename R, typename... Args>
        typename std::enable_if<std::is_void<P>::value && std::is_void<R>::value>::type
        async_send( uv_async_t *a, HandleData *hd ) {
            uv_async_send( a );
        };

        template <typename P, typename R>
        typename std::enable_if<std::is_void<P>::value && !std::is_void<R>::value, std::future<R>>

        ::type
        async_send( uv_async_t *a, HandleData *hd ) {
            typedef AsyncContinuation<void *, P, R> Cont;

            Cont *c = static_cast<Cont *>(hd->continuation.get());

            //Don't overwrite existing promise
            if( !c->r ) {
                c->r = std::make_shared<std::promise<R >>();
            }

            uv_async_send( a );

            return c->r->get_future();
        };

        template <typename P, typename R>
        typename std::enable_if<!std::is_void<P>::value && std::is_void<R>::value>::type
        async_send( uv_async_t *a, HandleData *hd, P &&arg ) {
            typedef AsyncContinuation<void *, P, R> Cont;

            Cont *c = static_cast<Cont *>(hd->continuation.get());

            c->p = std::make_shared<P>( std::move( arg ));

            uv_async_send( a );
        };

        template <typename P, typename R>
        typename std::enable_if<!std::is_void<P>::value && !std::is_void<R>::value, std::future<R>>

        ::type
        async_send( uv_async_t *a, HandleData *hd, P &&arg ) {
            typedef AsyncContinuation<void *, P, R> Cont;

            Cont *c = static_cast<Cont *>(hd->continuation.get());

            if( !c->r ) {
                c->r = std::make_shared<std::promise<R >>();
            }

            c->p = std::make_shared<P>( std::move( arg ));

            uv_async_send( a );

            return c->r->get_future();
        };


        template <typename P, typename R>
        typename std::enable_if<!std::is_void<P>::value && std::is_void<R>::value>::type
        async_send( uv_async_t *a, HandleData *hd, const P &arg ) {
            return async_send( a, hd, std::move( arg ));
        };

        template <typename P, typename R>
        typename std::enable_if<!std::is_void<P>::value && !std::is_void<R>::value, std::future<R>>

        ::type
        async_send( uv_async_t *a, HandleData *hd, const P &arg ) {
            return async_send( a, hd, std::move( arg ));
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
                uv::detail::start_async<P, R>( this->_loop, this->handle(), &this->internal_data, f );
            }

            template <typename... Args>
            auto send( Args &&... args ) {
                return detail::async_send<P, R>( this->handle(), &this->internal_data, std::forward<Args>( args )... );
            }

            void stop() {
                this->stop( []( auto ) {} );
            }

            template <typename Functor2>
            inline void stop( Functor2 f ) {
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
