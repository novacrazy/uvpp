//
// Created by Aaron on 8/6/2016.
//

#ifndef UV_BASE_HPP
#define UV_BASE_HPP

#include "../fwd.hpp"

#include <thread>

namespace uv {
    namespace detail {
        class FromLoop {
            protected:
                uv_loop_t *_uv_loop;
                Loop      *_parent_loop;

                void _loop_init( Loop *l ) noexcept;

                inline void _loop_init( Loop *l, uv_loop_t *ul ) noexcept {
                    assert( l != nullptr );
                    assert( ul != nullptr );

                    this->_parent_loop = l;
                    this->_uv_loop     = ul;
                }

            public:
                std::thread::id loop_thread() const noexcept;

                inline Loop *loop() noexcept {
                    return this->_parent_loop;
                }

                inline Loop const *loop() const noexcept {
                    return this->_parent_loop;
                }
        };
    }
}

#endif //UV_BASE_HPP
