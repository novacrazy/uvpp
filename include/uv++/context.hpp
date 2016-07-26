//
// Created by Aaron on 7/25/2016.
//

#ifndef UV_CONTEXT_HPP
#define UV_CONTEXT_HPP

namespace uv {
    namespace detail {
        template <typename Functor>
        struct ContextBase {
            void    *data;
            Functor f;

            ContextBase( void *d, Functor _f ) : data( d ), f( _f ) {}
        };
    }
}

#endif //UV_CONTEXT_HPP
