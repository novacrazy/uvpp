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
            inline void _init() {
                uv_check_init( this->_uv_loop, &_handle );
            }

            inline void _stop() {
                uv_check_stop( &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor, Check> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_check_start( this->handle(), []( uv_check_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Check *self = static_cast<Check *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
                } );
            }
    };
}

#endif //UV_CHECK_HANDLE_HPP
