//
// Created by Aaron on 7/29/2016.
//

#ifndef UV_REQUEST_HPP
#define UV_REQUEST_HPP

#include "fwd.hpp"

#include "exception.hpp"

#include "detail/type_traits.hpp"
#include "detail/data.hpp"

#include <future>
#include <memory>

namespace uv {
    struct RequestData : detail::UserData {

    };

    template <typename R>
    class Request : public std::enable_shared_from_this<Request<R>>,
                    public detail::UserDataAccess<RequestData, R> {
        public:
            typedef typename detail::UserDataAccess<RequestData, R>::handle_t handle_t;

        protected:
            RequestData internal_data;
    };

}

#endif //UV_REQUEST_HPP
