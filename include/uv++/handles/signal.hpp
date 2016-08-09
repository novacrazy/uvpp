//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_SIGNAL_HANDLE_HPP
#define UV_SIGNAL_HANDLE_HPP

#include "base.hpp"

namespace uv {

    class Signal final : public Handle<uv_signal_t, Signal> {
        public:
            typedef typename Handle<uv_signal_t, Signal>::handle_t handle_t;

        protected:
            void _init() noexcept {
                uv_signal_init( this->_uv_loop, &_handle );
            }

            void _stop() noexcept {
                uv_signal_stop( &_handle );
            }

        public:
            template <typename Functor>
            inline void start( int signum, Functor f ) {
                typedef detail::Continuation<Functor, Signal> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_signal_start( this->handle(), []( uv_signal_t *h, int sn ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Signal *self = static_cast<Signal *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self, sn );
                }, signum );
            }

            std::string signame() const noexcept {
                return detail::signame( this->handle()->signum );
            }
    };
}

#endif //UV_SIGNAL_HANDLE_HPP
