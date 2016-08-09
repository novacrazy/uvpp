//
// Created by Aaron on 7/29/2016.
//

#ifndef UV_DATA_DETAIL_HPP
#define UV_DATA_DETAIL_HPP

#include "../fwd.hpp"

#include <memory>
#include <type_traits>

namespace uv {
    namespace detail {
        struct UserData {
            //For user data, obviously
            std::shared_ptr<void> user_data;
        };

        template <typename D, typename H>
        class UserDataAccess {
                static_assert( std::is_base_of<UserData, D>::value, "HandleData must inherit from UserData" );

            protected:
                typedef D HandleData;

            public:
                typedef H handle_t;

                virtual const handle_t *handle() const noexcept = 0;

                virtual handle_t *handle() noexcept = 0;

                inline std::shared_ptr<void> &data() {
                    HandleData *p = static_cast<HandleData *>(this->handle()->data);

                    assert( p != nullptr );

                    return p->user_data;
                }

                template <typename R = void>
                inline const std::shared_ptr<R> data() const {
                    const HandleData *p = static_cast<HandleData *>(this->handle()->data);

                    assert( p != nullptr );

                    return std::static_pointer_cast<R>( p->user_data );
                }

                template <typename R = void>
                inline std::shared_ptr<R> data() {
                    const HandleData *p = static_cast<HandleData *>(this->handle()->data);

                    assert( p != nullptr );

                    return std::static_pointer_cast<R>( p->user_data );
                }
        };
    }
}

#endif //UV_DATA_DETAIL_HPP
