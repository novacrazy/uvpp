//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_WORK_REQUEST_HPP
#define UV_WORK_REQUEST_HPP

#include "base.hpp"

#include "../detail/async.hpp"

#include <cstdlib>

namespace uv {
    namespace detail {
        template <typename Functor, typename Self>
        struct WorkContinuation : public AsyncContinuation<Functor, Self> {
            WorkContinuation( Functor f ) noexcept
                : AsyncContinuation<Functor, Self>( f ) {
            }

            /*
             * This is so the deferred future can be returned when after_work_cb is invoked instead of immediately
             * when the result promise is set
             * */
            std::promise<void> finished;
        };


        struct NumWorkers : LazyStatic<size_t> {
            size_t init() noexcept {
                const char *val = std::getenv( "UV_THREADPOOL_SIZE" );

                /*
                 * This is the same logic used in:
                 * https://github.com/libuv/libuv/blob/b12624c13693c4d29ca84b3556eadc9e9c0936a4/src/threadpool.c#L146-L153
                 * */
                if( val != nullptr ) {
                    //1 is the minimum and 128 is the maximum as defined by libuv
                    return clamp<size_t>( std::stoull( val ), 1, 128 );

                } else {
                    return 4; //default threadpool size for libuv
                }
            }
        };
    }

    class Work : public Request<uv_work_t, Work> {
        public:
            typedef typename Request<uv_work_t, Work>::request_t request_t;

        protected:
            typedef typename Request<uv_work_t, Work>::RequestData RequestData;

            inline void _init() noexcept {
                //No-op
            }

            inline void _stop() noexcept {
                this->cancel();
            }

        private:
            template <typename Cont>
            void do_queue() {
                uv_queue_work( this->loop_handle(), this->request(), []( uv_work_t *w ) {
                    std::weak_ptr<RequestData> *d = static_cast<std::weak_ptr<RequestData> *>(w->data);

                    if( d != nullptr ) {
                        if( auto data = d->lock()) {
                            if( auto self = data->self.lock()) {
                                int expect_pending = REQUEST_PENDING;

                                self->_status.compare_exchange_strong( expect_pending, REQUEST_ACTIVE );

                                if( expect_pending == REQUEST_PENDING ) {
                                    data->cont<Cont>()->dispatch();
                                }
                            }

                        } else {
                            RequestData::cleanup( w, d );
                        }
                    }

                }, []( uv_work_t *w, int status ) {
                    std::weak_ptr<RequestData> *d = static_cast<std::weak_ptr<RequestData> *>(w->data);

                    if( d != nullptr ) {
                        if( auto data = d->lock()) {
                            if( auto self = data->self.lock()) {
                                int expect_active = REQUEST_ACTIVE;

                                self->_status.compare_exchange_strong( expect_active, REQUEST_FINISHED );

                                auto sc = data->cont<Cont>();

                                if( status != 0 ) {
                                    sc->finished.set_exception( std::make_exception_ptr( ::uv::Exception( status )));

                                } else if( expect_active != REQUEST_ACTIVE ) {
                                    //TODO: Better error message on this
                                    sc->finished.set_exception( std::make_exception_ptr( ::uv::Exception( "invalid state" )));

                                } else {
                                    sc->finished.set_value();
                                }
                            }
                        } else {
                            RequestData::cleanup( w, d );
                        }
                    }
                } );
            }

        public:
            static size_t num_workers() noexcept {
                static detail::NumWorkers num;

                return num;
            }

            inline void start() noexcept {
                //No-op
            }

            template <typename Functor, typename... Args>
            std::future<detail::result_of<Functor>> queue( Functor f, Args... args ) {
                typedef detail::function_traits<Functor>        ft;
                typedef typename ft::result_type                result_type;
                typedef detail::WorkContinuation<Functor, Work> Cont;

                static_assert( ft::arity >= sizeof...( Args ));

                /*
                 * This is so freaking cheating...
                 *
                 * Because REQUEST_PENDING is zero, AND-ing the status with REQUEST_ACTIVE will not affect it if it's
                 * active, but if it's in some other state, it'll "set" it to REQUEST_PENDING since the result of AND
                 * with any of the other status's will be zero.
                 * */
                int last_status = _status.fetch_and( REQUEST_ACTIVE );

                if( last_status == REQUEST_ACTIVE ) {
                    throw ::uv::Exception( UV_EBUSY );

                } else {
                    auto c = std::make_shared<Cont>( f );

                    c->init( this->shared_from_this(), std::forward<Args>( args )... );

                    this->internal_data->continuation = c;

                    if( last_status != REQUEST_PENDING ) {
                        if( !this->on_loop_thread()) {
                            schedule( this->loop(), [this] {
                                this->do_queue<Cont>();
                            } );

                        } else {
                            this->do_queue<Cont>();
                        }
                    }

                    auto result = c->s;

                    //I love this line. So succinct.
                    return util::then( c->finished, [result] { return *result; }, UV_ASYNC_LAUNCH );
                }
            }

            template <typename Functor, typename... Args>
            inline std::future<detail::result_of<Functor>> defer_queue( Functor f, Args... args ) {
                return std::async( std::launch::deferred, [this]( Functor inner_f, Args &&... inner_args ) {
                    return this->queue( inner_f, std::forward<Args>( inner_args )... ).get();
                }, f, std::forward<Args>( args )... );
            };
    };
}

#endif //UV_WORK_REQUEST_HPP
