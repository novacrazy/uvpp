//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_FWD_HPP
#define UV_FWD_HPP

#include "defines.hpp"

namespace uv {
    template <typename>
    class HandleBase;

    template <typename>
    class Handle;

    class Idle;

    class Prepare;

    class Check;

    class Loop;

    class Timer;

    template <typename, typename>
    class Async;

    class Signal;

    class Filesystem;

    class File;
}

#endif //UV_FWD_HPP
