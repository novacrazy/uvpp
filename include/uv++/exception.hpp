//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_EXCEPTION_HPP
#define UV_EXCEPTION_HPP

#include "defines.hpp"

namespace uv {
    class Exception : public std::exception {
        private:
            const char *__what;

        public:
            inline Exception( const char *w ) noexcept : __what( w ) {}

            inline Exception( int e ) noexcept : Exception( uv_strerror( e )) {}

            inline const char *what() const noexcept {
                return this->__what;
            }
    };
}

#endif //UV_EXCEPTION_HPP
