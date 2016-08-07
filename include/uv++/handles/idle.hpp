//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_IDLE_HANDLE_HPP
#define UV_IDLE_HANDLE_HPP

#include "base.hpp"

namespace uv {
    class Idle final : public Handle<uv_idle_t, Idle> {
        public:
            typedef typename Handle<uv_idle_t, Idle>::handle_t handle_t;

        protected:
            inline void _init() {
                uv_idle_init( this->_uv_loop, &_handle );
            }

            inline void _stop() {
                uv_idle_stop( &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor, Idle> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_idle_start( this->handle(), []( uv_idle_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Idle *self = static_cast<Idle *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
                } );
            }
    };
}

#endif //UV_IDLE_HANDLE_HPP
