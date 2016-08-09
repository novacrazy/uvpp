//
// Created by Aaron on 8/7/2016.
//

#ifndef UV_FS_REQUEST_HPP
#define UV_FS_REQUEST_HPP

#include "base.hpp"

#include "../detail/async.hpp"

namespace uv {
    namespace detail {
        void handle_fs_req( uv_fs_t *req, int status ) noexcept {

        }
    }

    namespace fs {
        class FSRequest : public Request<uv_fs_t, FSRequest> {
            public:
                typedef typename Request<uv_fs_t, FSRequest>::request_t request_t;

            protected:
                friend class Filesystem;

                template <typename>
                friend
                class FSResult;

                inline void _init() noexcept {
                    //No-op
                }

                inline void _stop() noexcept {
                    this->cancel();
                }

                template <typename Functor, typename... Args>
                std::future<request_t *> promisify( Functor uf, Args... args ) {
                    this->_status = REQUEST_PENDING;

                    auto r = std::make_shared<std::promise<request_t *>>();

                    this->internal_data.continuation = r;

                    schedule( this->loop(), [uf, this]( Args... inner_args ) -> void {
                        uf( this->_uv_loop, this->request(), std::forward<Args>( inner_args )..., []( uv_fs_t *req ) {
                            RequestData *d = static_cast<RequestData *>(req->data);

                            FSRequest *self = static_cast<FSRequest *>(d->self);

                            int expect_pending = REQUEST_PENDING;

                            self->_status.compare_exchange_strong( expect_pending, REQUEST_FINISHED );

                            auto *p = static_cast<std::promise<request_t *> *>(d->continuation.get());

                            if( expect_pending == REQUEST_PENDING ) {
                                int res = (int)req->result;

                                if( res < 0 ) {
                                    uv_fs_req_cleanup( req );

                                    p->set_exception( std::make_exception_ptr( ::uv::Exception( res )));

                                } else {
                                    p->set_value( req );
                                }

                            } else {
                                uv_fs_req_cleanup( req );

                                if( expect_pending == REQUEST_CANCELLED ) {
                                    p->set_exception( std::make_exception_ptr( ::uv::Exception( UV_ECANCELED )));

                                } else {
                                    p->set_exception( std::make_exception_ptr( ::uv::Exception( UV_UNKNOWN )));
                                }
                            }
                        } );
                    }, std::forward<Args>( args )... );

                    return r->get_future();
                }

                inline void start() noexcept {
                    //No-op
                }
        };
    }
}

#endif //UV_FS_REQUEST_HPP
