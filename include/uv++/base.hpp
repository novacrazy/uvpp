//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_BASE_HPP
#define UV_BASE_HPP

#include "defines.hpp"
#include "exception.hpp"
#include "fwd.hpp"

#include <memory>

namespace uv {
    template <typename Functor>
    struct Continuation : public std::enable_shared_from_this<Continuation<Functor>> {
        Functor f;

        Continuation( Functor _f ) : f( _f ) {}
    };
}

#endif //UV_BASE_HPP
