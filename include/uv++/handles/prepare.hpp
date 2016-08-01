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
            inline void _init() {
                uv_prepare_init( this->_loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor, Prepare> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_prepare_start( &_handle, []( uv_prepare_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Prepare *self = static_cast<Prepare *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
                } );
            }

            inline void stop() {
                uv_prepare_stop( &_handle );
            }
    };
}

#endif //UV_PREPARE_HANDLE_HPP
