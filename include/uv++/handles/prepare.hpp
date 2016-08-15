//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_PREPARE_HANDLE_HPP
#define UV_PREPARE_HANDLE_HPP

#include "base.hpp"

namespace uv {
    class Prepare final : public Handle<uv_prepare_t, Prepare> {
        public:
            typedef typename Handle<uv_prepare_t, Prepare>::handle_t handle_t;

        protected:
            typedef typename Handle<uv_prepare_t, Prepare>::HandleData HandleData;

            inline void _init() noexcept {
                uv_prepare_init( this->loop_handle(), this->handle());
            }

            inline void _stop() noexcept {
                uv_prepare_stop( this->handle());
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor, Prepare> Cont;

                this->internal_data->continuation = std::make_shared<Cont>( f );

                uv_prepare_start( this->handle(), []( uv_prepare_t *h ) {
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

#endif //UV_PREPARE_HANDLE_HPP
