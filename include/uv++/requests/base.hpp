//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_BASE_REQUEST_HPP
#define UV_BASE_REQUEST_HPP


#include "../exception.hpp"
#include "../detail/async.hpp"

#include "../detail/data.hpp"

#include <atomic>

namespace uv {
    struct RequestData : detail::UserData {
        std::shared_ptr<void> continuation;

        void *self;

        RequestData( void *s )
            : self( s ) {
            assert( s != nullptr );
        }
    };

    template <typename R, typename D>
    class Request : public std::enable_shared_from_this<Request<R, D>>,
                    public detail::UserDataAccess<RequestData, R>,
                    public detail::FromLoop {
        public:
            typedef typename detail::UserDataAccess<RequestData, R>::handle_t request_t;
            typedef D                                                         derived_type;

            enum class request_type : std::underlying_type<uv_req_type>::type {
                    UNKNOWN_REQ = 0,
#define XX( uc, lc ) uc = UV_##uc,
                    UV_REQ_TYPE_MAP( XX )
#undef XX
                    ACCEPT       = UV_ACCEPT,
                    FS_EVENT_REQ = UV_FS_EVENT_REQ,
                    POLL_REQ     = UV_POLL_REQ,
                    PROCESS_EXIT = UV_PROCESS_EXIT,
                    READ         = UV_READ,
                    UDP_RECV     = UV_UDP_RECV,
                    WAKEUP       = UV_WAKEUP,
                    SIGNAL_REQ   = UV_SIGNAL_REQ,
                    REQ_TYPE_MAX = UV_REQ_TYPE_MAX
            };

            enum {
                REQUEST_PENDING   = 0,
                REQUEST_IDLE      = ( 1 << 0 ),
                REQUEST_ACTIVE    = ( 1 << 1 ),
                REQUEST_CANCELLED = ( 1 << 2 ),
                REQUEST_FINISHED  = ( 1 << 3 )
            };

        protected:
            RequestData internal_data;

            request_t       _request;
            std::atomic_int _status;

            //Implemented in derived classes
            virtual void _init() = 0;

            void init( Loop *p, uv_loop_t *l ) {
                assert( p != nullptr );
                assert( l != nullptr );

                this->_parent_loop = p;
                this->_uv_loop     = l;

                this->handle()->data = &this->internal_data;

                this->_init();
            }

        private:
            typedef typename detail::UserDataAccess<RequestData, R>::handle_t handle_t;

            inline const handle_t *handle() const noexcept {
                return this->request();
            }

            inline handle_t *handle() noexcept {
                return this->request();
            }

        public:
            inline Request() noexcept
                : internal_data( this ), _status( REQUEST_IDLE ) {
            }

            //Implemented in Loop.hpp to pass Loop::handle() to this->init(uv_loop_t*)
            inline void init( Loop * );

            inline std::shared_future<void> cancel() {
                if( std::this_thread::get_id() == this->loop_thread()) {
                    if( this->_status == REQUEST_ACTIVE ) {
                        return detail::make_exception_future<void>( ::uv::Exception( UV_EBUSY ));

                    } else {
                        int res = uv_cancel((uv_req_t *)this->handle());

                        if( res != 0 ) {
                            return detail::make_exception_future<void>( ::uv::Exception( res ));

                        } else {
                            this->_status = REQUEST_CANCELLED;

                            return detail::make_ready_future();
                        }
                    }

                } else {
                    return detail::schedule( this->loop(), [this] {
                        return this->cancel().get();
                    } );
                }
            }

            inline int status() const noexcept {
                return this->_status;
            }

            inline bool is_pending() const noexcept {
                return this->_status == REQUEST_PENDING;
            }

            inline bool is_idle() const noexcept {
                return this->_status == REQUEST_IDLE;
            }

            inline bool is_cancelled() const noexcept {
                return this->_status == REQUEST_CANCELLED;
            }

            inline bool is_active() const noexcept {
                return this->_status == REQUEST_ACTIVE;
            }

            inline bool is_finished() const noexcept {
                return this->_status == REQUEST_FINISHED;
            }

            inline size_t size() noexcept {
                return uv_req_size( this->request()->type );
            }

            inline const request_t *request() const noexcept {
                return &_request;
            }

            inline request_t *request() noexcept {
                return &_request;
            }

            std::string name() const noexcept {
                switch((request_type)this->request()->type ) {
#define XX( uc, lc ) case request_type::uc: return #uc;
                    UV_REQ_TYPE_MAP( XX )
                    XX( ACCEPT, accept )
                    XX( FS_EVENT_REQ, fs_event_req )
                    XX( POLL_REQ, poll_req )
                    XX( PROCESS_EXIT, process_exit )
                    XX( READ, read )
                    XX( UDP_RECV, udp_recv )
                    XX( WAKEUP, wakeup )
                    XX( SIGNAL_REQ, signal_req )
#undef XX
                    default:
                        return "UNKNOWN_REQ";
                }
            }
    };
}

#endif //UV_BASE_REQUEST_HPP
