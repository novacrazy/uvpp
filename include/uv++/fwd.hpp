//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_FWD_HPP
#define UV_FWD_HPP

#include "defines.hpp"

#include <iosfwd>

namespace uv {
    class Exception;

    template <typename, typename>
    class HandleBase;

    template <typename, typename>
    class Handle;

    class Idle;

    class Prepare;

    class Check;

    class Loop;

    class Timer;

    class Async;

    template <typename>
    class AsyncDetail;

    class Signal;

    template <typename, typename>
    class Request;

    class Work;

    class MultiWork;

    namespace fs {
        class File;

        class Stat;

        class Filesystem;

        class FSRequest;

        template <typename>
        class FSResult;
    }

    namespace os {
        struct cpu_info_t;

        struct passwd_t;
    }

    namespace net {
        struct interface_t;
    }

    template <typename... Args>
    inline UV_DECLTYPE_AUTO schedule( Loop *, Args... );
}

#endif //UV_FWD_HPP
