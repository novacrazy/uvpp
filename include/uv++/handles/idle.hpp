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
            typedef typename Handle<uv_idle_t, Idle>::HandleData HandleData;

            inline void _init() noexcept {
                uv_idle_init( this->loop_handle(), this->handle());
            }

            inline void _stop() noexcept {
                uv_idle_stop( this->handle());
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor, Idle> Cont;

                this->internal_data->continuation = std::make_shared<Cont>( f );

                uv_idle_start( this->handle(), []( uv_idle_t *h ) {
                    std::weak_ptr<HandleData> *d = static_cast<std::weak_ptr<HandleData> *>(h->data);

                    if( d != nullptr ) {
                        if( auto data = d->lock()) {
                            if( auto self = data->self.lock()) {
                                data->cont<Cont>()->dispatch( self );
                            }

                        } else {
                            HandleData::cleanup( h, d );
                        }
                    }
                } );
            }
    };
}

#endif //UV_IDLE_HANDLE_HPP
