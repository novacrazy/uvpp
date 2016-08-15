//
// Created by Aaron on 8/6/2016.
//

#ifndef UV_FROM_LOOP_DETAIL_HPP
#define UV_FROM_LOOP_DETAIL_HPP

#include "../fwd.hpp"
#include "../exception.hpp"

#include <thread>

namespace uv {
    namespace detail {
        class FromLoop {
            protected:
                std::weak_ptr<Loop> parent_loop;

                inline void _loop_init( std::shared_ptr<Loop> l ) noexcept {
                    assert( bool(l));

                    this->parent_loop = l;
                }

                uv_loop_t *loop_handle();

            public:
                std::thread::id loop_thread() const;

                bool on_loop_thread() const;

                std::shared_ptr<Loop> loop() {
                    if( auto l = this->parent_loop.lock()) {
                        return l;

                    } else {
                        throw ::uv::Exception( "Owning loop has been destroyed" );
                    }
                }

                const std::shared_ptr<Loop> loop() const {
                    if( auto l = this->parent_loop.lock()) {
                        return l;

                    } else {
                        throw ::uv::Exception( "Owning loop has been destroyed" );
                    }
                }

                inline bool has_loop() const noexcept {
                    return !this->parent_loop.expired();
                }
        };
    }
}

#endif //UV_FROM_LOOP_DETAIL_HPP
