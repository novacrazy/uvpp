//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_CHECK_HANDLE_HPP
#define UV_CHECK_HANDLE_HPP

#include "base.hpp"

namespace uv {
    class Check final : public Handle<uv_check_t, Check> {
        public:
            typedef typename Handle<uv_check_t, Check>::handle_t handle_t;

        protected:
            typedef typename Handle<uv_check_t, Check>::HandleData HandleData;

            inline void _init() noexcept {
                uv_check_init( this->loop_handle(), this->handle());
            }

            inline void _stop() noexcept {
                uv_check_stop( this->handle());
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor, Check> Cont;

                this->internal_data->continuation = std::make_shared<Cont>( f );

                uv_check_start( this->handle(), []( uv_check_t *h ) {
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

#endif //UV_CHECK_HANDLE_HPP
