//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_HANDLE_HPP
#define UV_HANDLE_HPP

#include "fwd.hpp"

#include "defines.hpp"
#include "exception.hpp"
#include "type_traits.hpp"

#include "detail/handle_detail.hpp"

#include <memory>
#include <future>

namespace uv {
    struct HandleData {
        //For user data, obviously
        std::shared_ptr<void> user_data;

        //For primary continuation of callbacks
        std::shared_ptr<void> continuation;

        //Only used for close callbacks
        std::shared_ptr<void> secondary_continuation;

        void *self;

        HandleData( void *s ) : self( s ) {
            assert( s != nullptr );
        }
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

            uv_loop_t *_loop;
            Loop      *_parent_loop;

            //Implemented in derived classes
            virtual void _init() = 0;

            //Can be called in subclasses
            inline void init( Loop *p, uv_loop_t *l ) {
                assert( p != nullptr );
                assert( l != nullptr );

                this->_parent_loop = p;
                this->_loop        = l;

                this->handle()->data = &this->internal_data;

                this->_init();
            }

        public:
            HandleBase() : internal_data( this ) {}

            //Implemented in Loop.hpp to pass Loop::handle() to this->init(uv_loop_t*)
            inline void init( Loop * );

            virtual const handle_t *handle() const = 0;

            virtual handle_t *handle() = 0;

            virtual void stop() = 0;

            virtual void start() {
                throw new Exception( UV_ENOSYS );
            };

            inline Loop *loop() {
                return this->_parent_loop;
            }

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

    template <typename H, typename D>
    class Handle : public HandleBase<H> {
        public:
            typedef typename HandleBase<H>::handle_t handle_t;
            typedef D                                derived_type;

        protected:
            handle_t _handle;

        public:
            inline const handle_t *handle() const {
                return &_handle;
            }

            inline handle_t *handle() {
                return &_handle;
            }

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
            std::shared_future<void> close( Functor );
    };

    class Idle final : public Handle<uv_idle_t, Idle> {
        public:
            typedef typename Handle<uv_idle_t, Idle>::handle_t handle_t;

        protected:
            inline void _init() {
                uv_idle_init( this->_loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_idle_start( &_handle, []( uv_idle_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Idle *self = static_cast<Idle *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->f( *self );
                } );
            }

            inline void stop() {
                uv_idle_stop( &_handle );
            }
    };

    class Prepare final : public Handle<uv_prepare_t, Prepare> {
        public:
            typedef typename Handle<uv_prepare_t, Prepare>::handle_t handle_t;

        protected:
            inline void _init() {
                uv_prepare_init( this->_loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_prepare_start( &_handle, []( uv_prepare_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Prepare *self = static_cast<Prepare *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->f( *self );
                } );
            }

            inline void stop() {
                uv_prepare_stop( &_handle );
            }
    };

    class Check final : public Handle<uv_check_t, Check> {
        public:
            typedef typename Handle<uv_check_t, Check>::handle_t handle_t;

        protected:
            inline void _init() {
                uv_check_init( this->_loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( Functor f ) {
                typedef detail::Continuation<Functor> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_check_start( &_handle, []( uv_check_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Check *self = static_cast<Check *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->f( *self );
                } );
            }

            inline void stop() {
                uv_check_stop( &_handle );
            }
    };

    class Timer final : public Handle<uv_timer_t, Timer> {
        public:
            typedef typename Handle<uv_timer_t, Timer>::handle_t handle_t;

        protected:
            inline void _init() {
                uv_timer_init( this->_loop, &_handle );
            }

        public:
            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline void start( Functor f,
                               const std::chrono::duration<_Rep, _Period> &timeout,
                               const std::chrono::duration<_Rep2, _Period2> &repeat =
                               std::chrono::duration<_Rep2, _Period2>(
                                   std::chrono::duration_values<_Rep2>::zero())) {

                typedef std::chrono::duration<uint64_t, std::milli> millis;

                typedef detail::Continuation<Functor> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_timer_start( &_handle, []( uv_timer_t *h ) {
                                    HandleData *d = static_cast<HandleData *>(h->data);

                                    Check *self = static_cast<Check *>(d->self);

                                    static_cast<Cont *>(d->continuation.get())->f( *self );
                                },
                    //libuv expects milliseconds, so convert any duration given to milliseconds
                                std::chrono::duration_cast<millis>( timeout ).count(),
                                std::chrono::duration_cast<millis>( repeat ).count()
                );
            }

            inline void stop() {
                uv_timer_stop( &_handle );
            }
    };

    class Signal final : public Handle<uv_signal_t, Signal> {
        public:
            typedef typename Handle<uv_signal_t, Signal>::handle_t handle_t;

        protected:
            void _init() {
                uv_signal_init( this->_loop, &_handle );
            }

        public:
            template <typename Functor>
            inline void start( int signum, Functor f ) {
                typedef detail::Continuation<Functor> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_signal_start( &_handle, []( uv_signal_t *h, int sn ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Signal *self = static_cast<Signal *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->f( *self, sn );
                }, signum );
            }

            std::string signame() const {
                return detail::signame( _handle.signum );
            }

            inline void stop() {
                uv_signal_stop( &_handle );
            }
    };

    template <typename D>
    struct HandleHash {
        typedef HandleBase<typename D::handle_t> *argument_type;
        typedef std::size_t                      result_type;

        result_type operator()( argument_type const &h ) const {
            return reinterpret_cast<size_t>(h->handle());
        }
    };
}

#endif //UV_HANDLE_HPP
