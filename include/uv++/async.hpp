//
// Created by Aaron on 7/28/2016.
//

#ifndef UV_ASYNC_HPP
#define UV_ASYNC_HPP

#include "handle.hpp"

#include "detail/async_detail.hpp"

namespace uv {
    template <typename P = void, typename R = void>
    class Async final : public Handle<uv_async_t, Async<P, R>> {
        public:
            typedef typename Handle<uv_async_t, Async<P, R>>::handle_t handle_t;

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

                    c->dispatch( static_cast<Async<P, R> *>(d->self));
                } );
            }

            template <typename... Args>
            inline std::shared_future<R> send( Args &&... args ) {
                typedef detail::AsyncContinuation<void *, P, R> Cont;

                Cont *c = static_cast<Cont *>(this->internal_data.continuation.get());

                std::atomic_bool should_send( true );

                auto ret = c->init( should_send, std::forward<Args>( args )... );

                if( should_send ) {
                    uv_async_send( this->handle());
                }

                return ret;
            }

            inline void stop() {
                this->close( []( auto & ) {} );
            }

            template <typename Functor>
            std::shared_future<void> stop( Functor f ) {
                return this->close( f );
            }

            ~Async() {
                if( this->is_active()) {
                    this->stop();
                }
            }
    };
}

#endif //UV_ASYNC_HPP
