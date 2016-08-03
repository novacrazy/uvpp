//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_WORK_REQUEST_HPP
#define UV_WORK_REQUEST_HPP

#include "base.hpp"

#include <cstdlib>

namespace uv {
    namespace detail {
        struct NumWorkers : LazyStatic<size_t> {
            void init() {
                const char *val = std::getenv( "UV_THREADPOOL_SIZE" );

                /*
                 * This is the same logic used in:
                 * https://github.com/libuv/libuv/blob/b12624c13693c4d29ca84b3556eadc9e9c0936a4/src/threadpool.c#L146-L153
                 * */
                if( val != nullptr ) {
                    value = std::stoull( val );

                    if( value == 0 ) {
                        value = 1;

                    } else if( value > 128 ) { //MAX_THREADPOOL_SIZE for libuv
                        value = 128;
                    }

                } else {
                    value = 4; //default threadpool size for libuv
                }
            }
        };
    }

    class Work : public Request<uv_work_t, Work> {
        public:
            typedef typename Request<uv_work_t, Work>::request_t request_t;

        protected:
            inline void _init() {
                //No-op
            }

            inline void _stop() {
                this->cancel();
            }

        public:
            static size_t num_workers() {
                static detail::NumWorkers num;

                return num;
            }
    };
}

#endif //UV_WORK_REQUEST_HPP
