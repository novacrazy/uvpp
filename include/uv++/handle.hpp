//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_HANDLE_HPP
#define UV_HANDLE_HPP

#include "fwd.hpp"
#include "base.hpp"

#include <memory>

namespace uv {
    struct HandleData {
        std::shared_ptr<void> user_data;
        std::shared_ptr<void> continuation;
        void                  *self;

        HandleData( void *s ) : self( s ) {}
    };

    template <typename H>
    class HandleBase : public std::enable_shared_from_this<HandleBase<H>> {
        public:
            typedef H handle_t;

            enum class handle_type : std::underlying_type<uv_handle_type>::type {
                    UNKNOWN_HANDLE = 0,
#define XX( uc, lc ) uc,
                    UV_HANDLE_TYPE_MAP( XX )
#undef XX
                    FILE,
                    HANDLE_TYPE_MAX
            };

        protected:
            HandleData internal_data;

            virtual void _init() {
                //No-op
            }

            void _initData() {
                this->handle()->data = &this->internal_data;
            }

        public:
            HandleBase() : internal_data( this ) {}

            virtual const handle_t *handle() const = 0;

            virtual handle_t *handle() = 0;

            virtual void stop() = 0;

            virtual void start() {
                throw new Exception( UV_ENOSYS );
            };

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

            handle_type guess_type() const {
                return (handle_type)uv_guess_handle( this->handle()->type );
            }
    };

    template <typename H>
    class Handle : public HandleBase<H> {
        protected:
            uv_loop_t *loop;

        public:
            typedef typename HandleBase<H>::handle_t handle_t;

            inline void init( Loop *l );

            inline bool is_active() const {
                return uv_is_active((uv_handle_t *)( this->handle())) != 0;
            }

            inline bool is_closing() const {
                return uv_is_closing((uv_handle_t *)( this->handle())) != 0;
            }

            inline size_t size() {
                return uv_handle_size( this->handle()->type );
            }

            template <typename Functor>
            void close( Functor f ) {

            }
    };

    class Idle : public Handle<uv_idle_t> {
        public:
            typedef typename Handle<uv_idle_t>::handle_t handle_t;

        private:
            uv_idle_t _handle;

        protected:
            inline void _init() {
                uv_idle_init( this->loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                this->internal_data.continuation = std::make_shared<Continuation<Functor>>( f );

                uv_idle_start( &_handle, []( uv_idle_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Idle *self = static_cast<Idle *>(d->self);

                    static_cast<Continuation<Functor> *>(d->continuation.get())->f( *self );
                } );
            }

            inline const handle_t *handle() const {
                return &_handle;
            }

            inline handle_t *handle() {
                return &_handle;
            }

            inline void stop() {
                uv_idle_stop( &_handle );
            }
    };

    class Prepare : public Handle<uv_prepare_t> {
        public:
        public:
            typedef typename Handle<uv_prepare_t>::handle_t handle_t;

        private:
            uv_prepare_t _handle;

        protected:
            inline void _init() {
                uv_prepare_init( this->loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                this->internal_data.continuation = std::make_shared<Continuation<Functor>>( f );

                uv_prepare_start( &_handle, []( uv_prepare_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Prepare *self = static_cast<Prepare *>(d->self);

                    static_cast<Continuation<Functor> *>(d->continuation.get())->f( *self );
                } );
            }

            inline void stop() {
                uv_prepare_stop( &_handle );
            }

            inline const handle_t *handle() const {
                return &_handle;
            }

            inline handle_t *handle() {
                return &_handle;
            }
    };

    class Check : public Handle<uv_check_t> {
        public:
            typedef typename Handle<uv_check_t>::handle_t handle_t;

        private:
            uv_check_t _handle;

        protected:
            inline void _init() {
                uv_check_init( this->loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                this->internal_data.continuation = std::make_shared<Continuation<Functor>>( f );

                uv_check_start( &_handle, []( uv_check_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Check *self = static_cast<Check *>(d->self);

                    static_cast<Continuation<Functor> *>(d->continuation.get())->f( *self );
                } );
            }

            inline const handle_t *handle() const {
                return &_handle;
            }

            inline handle_t *handle() {
                return &_handle;
            }

            inline void stop() {
                uv_check_stop( &_handle );
            }
    };

    class Timer : public Handle<uv_timer_t> {
        public:
            typedef typename Handle<uv_timer_t>::handle_t handle_t;

        private:
            uv_timer_t _handle;

        protected:
            inline void _init() {
                uv_timer_init( this->loop, &_handle );
            }

        public:
            template <typename Functor, typename _Rep, typename _Period, typename _Rep2, typename _Period2>
            inline void start( Functor f,
                               const std::chrono::duration<_Rep, _Period> &timeout,
                               const std::chrono::duration<_Rep2, _Period2> &repeat = std::chrono::duration_values<_Rep>::zero()) {
                typedef std::chrono::duration<uint64_t, std::milli> millis;

                this->internal_data.continuation = std::make_shared<Continuation<Functor>>( f );

                uv_timer_start( &_handle, []( uv_timer_t *h ) {
                                    HandleData *d = static_cast<HandleData *>(h->data);

                                    Check *self = static_cast<Check *>(d->self);

                                    static_cast<Continuation<Functor> *>(d->continuation.get())->f( *self );
                                },
                                std::chrono::duration_cast<millis>( timeout ).count(),
                                std::chrono::duration_cast<millis>( repeat ).count()
                );
            }

            inline const handle_t *handle() const {
                return &_handle;
            }

            inline handle_t *handle() {
                return &_handle;
            }

            inline void stop() {
                uv_timer_stop( &_handle );
            }
    };

    class Async : public Handle<uv_async_t> {
        public:
            typedef typename Handle<uv_async_t>::handle_t handle_t;

        private:
            uv_async_t _handle;

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                this->internal_data.continuation = std::make_shared<Continuation<Functor>>( f );

                uv_async_init( this->loop, &_handle, []( uv_async_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Check *self = static_cast<Check *>(d->self);

                    static_cast<Continuation<Functor> *>(d->continuation.get())->f( *self );
                } );
            }

            inline void send() {
                uv_async_send( &_handle );
            }

            inline const handle_t *handle() const {
                return &_handle;
            }

            inline handle_t *handle() {
                return &_handle;
            }

            void stop() {
                this->stop( []( auto ) {} );
            }

            template <typename Functor>
            inline void stop( Functor f ) {
                this->close( f );
            }
    };

    template <typename D>
    struct HandleHash {
        typedef HandleBase<typename D::handle_t> *argument_type;
        typedef std::size_t                         result_type;

        result_type operator()( argument_type const &h ) const {
            return reinterpret_cast<size_t>(h->handle());
        }
    };
}

#endif //UV_HANDLE_HPP
