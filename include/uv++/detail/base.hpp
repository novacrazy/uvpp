//
// Created by Aaron on 8/6/2016.
//

#ifndef UV_BASE_HPP
#define UV_BASE_HPP

#include "../fwd.hpp"
#include "../exception.hpp"

#include <thread>

namespace uv {
    namespace detail {
        class FromLoop {
            protected:
                std::weak_ptr<Loop> _parent_loop;

                inline void _loop_init( std::shared_ptr<Loop> l ) noexcept {
                    assert( bool(l));

                    this->_parent_loop = l;
                }

                uv_loop_t *loop_handle();

            public:
                std::thread::id loop_thread() const;

                std::shared_ptr<Loop> loop() {
                    if( auto l = this->_parent_loop.lock()) {
                        return l;

                    } else {
                        throw ::uv::Exception( "Owning loop has been destroyed" );
                    }
                }

                const std::shared_ptr<Loop> loop() const {
                    if( auto l = this->_parent_loop.lock()) {
                        return l;

                    } else {
                        throw ::uv::Exception( "Owning loop has been destroyed" );
                    }
                }
        };
    }
}

#endif //UV_BASE_HPP
