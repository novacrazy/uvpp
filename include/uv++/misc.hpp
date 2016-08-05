//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_MISC_HPP
#define UV_MISC_HPP

#include "fwd.hpp"

namespace uv {
    //I would still recommend the stuff in <chrono> though
    inline uint64_t hrtime() {
        return uv_hrtime();
    }
}

#endif //UV_MISC_HPP
