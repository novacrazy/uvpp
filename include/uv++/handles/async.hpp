//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_ASYNC_HANDLE_HPP
#define UV_ASYNC_HANDLE_HPP

#include "base.hpp"

#include "../detail/async.hpp"

namespace uv {
    namespace detail {
        template <typename, size_t>
        struct send_void_helper {
            template <typename A>
            static std::shared_future<void> send_void( A * ) {
                throw ::uv::Exception( "invalid async handle for send_void" );
            }
        };

        template <>
        struct send_void_helper<void, 0> {
            template <typename A>
            static std::shared_future<void> send_void( A *d ) {
                return d->send();
            }
        };
    }

    class Async : public Handle<uv_async_t, Async> {
        public:
            typedef typename Handle<uv_async_t, Async>::handle_t handle_t;

        protected:
            inline void _init() {
                //No-op for uv_async_t
            }

            inline void _stop() {
                //Also a no-op for uv_async_t
            }

        public:
            virtual std::shared_future<void> send_void() = 0;
    };

    template <typename Functor>
    class AsyncDetail final : public Async {
        public:
            typedef typename Async::handle_t handle_t;

        protected:
            std::mutex m;

            typedef detail::AsyncContinuation<Functor, Async> Continuation;

            typedef typename detail::function_traits<Functor>::result_type result_type;
            typedef typename detail::function_traits<Functor>::tuple_type  tuple_type;

            enum {
                arity = detail::function_traits<Functor>::arity -
                        ( detail::ContinuationNeedsSelf<Functor, Async>::value )
            };

            std::atomic_bool is_sending;

        public:
            inline void start( Functor f ) {
                this->internal_data.continuation = std::make_shared<Continuation>( f );

                this->is_sending = false;

                uv_async_init( this->_uv_loop, this->handle(), []( uv_async_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    auto *self = static_cast<AsyncDetail<Functor> *>(d->self);

                    std::lock_guard<std::mutex> lock( self->m );

                    Continuation *c = static_cast<Continuation *>(d->continuation.get());

                    if( self->closing ) {
                        c->set_exception( ::uv::Exception( "async handle has been closed" ));

                    } else {
                        c->dispatch();
                    }

                    self->is_sending = false;
                } );
            }

            /*
             * The enable_if is to generate slightly more appealing error messages when there are
             * incorrect number of arguments given. That way it fails here instead of deep into the details.
             * */
            template <typename... Args>
            typename std::enable_if<sizeof...( Args ) == arity, std::shared_future<result_type>>::type
            send( Args... args ) {
                /*
                 * The mutex is here because by the time it acquires the lock there is a low chance the Async handle
                 * will be closing.
                 * */
                std::lock_guard<std::mutex> lock( this->m );

                if( this->closing ) {
                    throw ::uv::Exception( "async handle closed" );

                } else {
                    bool expect_sending = false;

                    this->is_sending.compare_exchange_strong( expect_sending, true );

                    Continuation *c = static_cast<Continuation *>(this->internal_data.continuation.get());

                    auto ret = c->init( static_cast<Async *>(this), std::forward<Args>( args )... );

                    /*
                     * So even with the mutex and stuff above, there is a slight chance libuv will still occasionally
                     * try to run the async task twice if you queue it up many times really fast.
                     *
                     * I'm guessing it marks the task as "done" before it runs the callback, so in the time between
                     * the callback starting and it finishing, queueing up another uv_async_send could potentially
                     * cause it to run again. So just keep track of it with our own flag so it doesn't confuse it.
                     * */
                    if( !expect_sending ) {
                        uv_async_send( this->handle());
                    }

                    return ret;
                }
            }

            template <typename... Args>
            inline typename std::enable_if<sizeof...( Args ) == arity, std::future<result_type>>::type
            defer_send( Args... args ) {
                return std::async( std::launch::deferred, [this]( Args &&... inner_args ) {
                    return this->send( std::forward<Args>( inner_args )... ).get();
                }, std::forward<Args>( args )... );
            };

            inline std::shared_future<void> send_void() override {
                return detail::send_void_helper<result_type, arity>::send_void( this );
            }
    };
}

#endif //UV_ASYNC_HANDLE_HPP
