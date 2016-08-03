//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_BASE_REQUEST_HPP
#define UV_BASE_REQUEST_HPP


#include "../fwd.hpp"

#include "../exception.hpp"

#include "../detail/type_traits.hpp"
#include "../detail/data.hpp"

#include <future>
#include <memory>

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
                    public detail::UserDataAccess<RequestData, R> {
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

        protected:
            RequestData internal_data;

            uv_loop_t *_loop;
            Loop      *_parent_loop;

            request_t        _request;
            std::atomic_bool cancelled;

            //Implemented in derived classes
            virtual void _init() = 0;

            void init( Loop *p, uv_loop_t *l ) {
                assert( p != nullptr );
                assert( l != nullptr );

                this->_parent_loop = p;
                this->_loop        = l;

                this->handle()->data = &this->internal_data;

                this->_init();
            }

        private:
            typedef typename detail::UserDataAccess<RequestData, R>::handle_t handle_t;

            inline const handle_t *handle() const {
                return this->request();
            }

            inline handle_t *handle() {
                return this->request();
            }

        public:
            inline Request() : internal_data( this ) {}

            //Implemented in Loop.hpp to pass Loop::handle() to this->init(uv_loop_t*)
            inline void init( Loop * );

            std::thread::id loop_thread();

            inline void cancel() {
                assert( std::this_thread::get_id() == this->loop_thread());

                int res = uv_cancel((uv_req_t *)this->handle());

                if( res != 0 ) {
                    throw ::uv::Exception( res );
                }
            }

            inline size_t size() {
                return uv_req_size( this->request()->type );
            }

            inline Loop *loop() {
                return this->_parent_loop;
            }

            inline const request_t *request() const {
                return &_request;
            }

            inline request_t *request() {
                return &_request;
            }

            std::string name() const {
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
