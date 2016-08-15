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
    template <typename R, typename D>
    struct RequestDataT : detail::UserData {
        /*
         * Shared pointers are used for this because of their type-erased deleters.
         * */
        std::shared_ptr<void> continuation;

        /*
         * This is kept here to ensure a circular reference between the handle and the handle data
         * */
        std::shared_ptr<R> request;

        /*
         * This is weak, because even if the handle data is still alive (via the above reference), it's okay
         * for the owning Handle object to be destroyed.
         * */
        std::weak_ptr<D> self;

        /*
         * These are just here to make continuation code cleaner at usage sites
         * */

        template <typename Cont>
        inline Cont *cont() {
            return static_cast<Cont *>(this->continuation.get());
        }

        inline static void cleanup( R *r, std::weak_ptr<RequestDataT> *t ) {
            assert( r != nullptr );
            assert( t != nullptr );

            delete t;
            r->data = nullptr;
        }

        RequestDataT( std::shared_ptr<D> s, std::shared_ptr<R> r )
            : self( s ), request( r ) {
        }
    };

    template <typename R, typename D>
    class Request : public std::enable_shared_from_this<Request<R, D>>,
                    public detail::UserDataAccess<RequestDataT<R, D>, R>,
                    public detail::FromLoop {
        public:
            typedef typename detail::UserDataAccess<RequestDataT<R, D>, R>::handle_t   request_t;
            typedef D                                                                  derived_type;
            typedef typename detail::UserDataAccess<RequestDataT<R, D>, R>::HandleData RequestData;

            enum class request_kind : std::underlying_type<uv_req_type>::type {
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
            std::shared_ptr<RequestData> internal_data;

            std::shared_ptr<request_t> _request;
            std::atomic_int            _status;

            //Implemented in derived classes
            virtual void _init() = 0;

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
                : _request( new request_t ),
                  _status( REQUEST_IDLE ) {
            }

            inline void init( std::shared_ptr<Loop> l ) {
                this->_loop_init( l );

                this->internal_data = std::make_shared<RequestData>( std::static_pointer_cast<derived_type>( this->shared_from_this()), this->_request );

                this->handle()->data = new std::weak_ptr<RequestData>( this->internal_data );

                this->_init();
            }

            inline std::shared_future<void> cancel() {
                if( this->on_loop_thread()) {
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
                    return schedule( this->loop(), [this] {
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
                return _request.get();
            }

            inline request_t *request() noexcept {
                return _request.get();
            }

            std::string name() const noexcept {
                switch((request_kind)this->request()->type ) {
#define XX( uc, lc ) case request_kind::uc: return #uc;
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

            ~Request() {

            }
    };
}

#endif //UV_BASE_REQUEST_HPP
