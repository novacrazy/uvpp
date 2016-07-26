//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_DEFINES_HPP
#define UV_DEFINES_HPP

#include <uv.h>
#include <fcntl.h>
#include <cassert>

#include <exception>

#ifndef UV_READ_BUFFER_SIZE
#define UV_READ_BUFFER_SIZE 16384 //16k
#endif

#ifndef UV_WRITE_BUFFER_SIZE
#define UV_WRITE_BUFFER_SIZE 16384 //16k
#endif

#endif //UV_DEFINES_HPP
