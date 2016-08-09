//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_DEFINES_HPP
#define UV_DEFINES_HPP

#if __cplusplus >= 201500L
# define UV_HAS_CPP17
#elif __cplusplus < 201402L
# error "This library requires C++14 or higher."
#endif

#include <uv.h>
#include <fcntl.h>
#include <cassert>

#include <exception>

#ifndef UV_READ_BUFFER_SIZE
# define UV_READ_BUFFER_SIZE 16384 //16k
#endif

#ifndef UV_WRITE_BUFFER_SIZE
# define UV_WRITE_BUFFER_SIZE 16384 //16k
#endif

#ifdef UV_USE_BOOST_LOCKFREE
# ifndef UV_LOCKFREE_QUEUE_SIZE
#  define UV_LOCKFREE_QUEUE_SIZE 128
# endif
#endif

#ifndef UV_ASYNC_LAUNCH
# define UV_ASYNC_LAUNCH ::std::launch::deferred
#endif

#endif //UV_DEFINES_HPP
