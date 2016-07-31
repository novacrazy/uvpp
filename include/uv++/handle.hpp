//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_HANDLE_HPP
#define UV_HANDLE_HPP

#include "fwd.hpp"

#include "detail/data.hpp"

#include "exception.hpp"

#include "detail/type_traits.hpp"
#include "detail/handle.hpp"
#include "detail/async.hpp"

#include <future>

namespace uv {
    struct HandleData : detail::UserData {
        //For primary continuation of callbacks
        std::shared_ptr<void> continuation;

        //Only used for close callbacks
        std::shared_ptr<void> close_continuation;

        void *self;

        HandleData( void *s )
            : self( s ) {
            assert( s != nullptr );
        }
    };

    template <typename H, typename D>
    class HandleBase : public std::enable_shared_from_this<D>,
                       public detail::UserDataAccess<HandleData, H> {
        public:
            typedef typename detail::UserDataAccess<HandleData, H>::handle_t handle_t;
            typedef D                                                        derived_type;

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

            virtual void stop() = 0;

            virtual void start() {
                throw new ::uv::Exception( UV_ENOSYS );
            };

            inline Loop *loop() {
                return this->_parent_loop;
            }
    };

    template <typename H, typename D>
    class Handle : public HandleBase<H, D> {
        public:
            typedef typename HandleBase<H, D>::handle_t     handle_t;
            typedef typename HandleBase<H, D>::derived_type derived_type;

            enum class handle_type : std::underlying_type<uv_handle_type>::type {
                    UNKNOWN_HANDLE = 0,
#define XX( uc, lc ) uc = UV_##uc,
                    UV_HANDLE_TYPE_MAP( XX )
#undef XX
                    FILE,
                    HANDLE_TYPE_MAX
            };


        protected:
            handle_t         _handle;
            std::atomic_bool closing;

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
                return this->closing || uv_is_closing((uv_handle_t *)( this->handle())) != 0;
            }

            inline size_t size() {
                return uv_handle_size( this->handle()->type );
            }

            inline void stop() {
                this->close( []() {} );
            }

            template <typename Functor>
            inline std::future<void> stop( Functor f ) {
                return this->close( f );
            }

            template <typename Functor>
            std::future<void> close( Functor );

            inline handle_type guess_handle() const {
                return (handle_type)uv_guess_handle( this->handle()->type );
            }

            std::string name() const {
                switch((handle_type)this->handle()->type ) {
#define XX( uc, lc ) case handle_type::uc: return #uc;
                    UV_HANDLE_TYPE_MAP( XX )
                    XX( FILE, file )
                    XX( HANDLE_TYPE_MAX, handle_type_max )
                    default:
                        return "UNKNOWN_HANDLE";
                }
            }

            std::string guess_handle_name() const {
                switch( this->guess_handle()) {
                    UV_HANDLE_TYPE_MAP( XX )
                    XX( FILE, file )
                    XX( HANDLE_TYPE_MAX, handle_type_max )
#undef XX
                    default:
                        return "UNKNOWN_HANDLE";
                }
            }

            ~Handle() {
                if( !this->is_closing()) {
                    this->stop();
                }
            }
    };

    typedef Handle<uv_handle_t, void> VoidHandle;

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
                typedef detail::Continuation<Functor, Idle> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_idle_start( &_handle, []( uv_idle_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Idle *self = static_cast<Idle *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
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
                typedef detail::Continuation<Functor, Prepare> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_prepare_start( &_handle, []( uv_prepare_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Prepare *self = static_cast<Prepare *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
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
                typedef detail::Continuation<Functor, Check> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_check_start( &_handle, []( uv_check_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Check *self = static_cast<Check *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
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

                typedef detail::Continuation<Functor, Timer> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_timer_start( &_handle, []( uv_timer_t *h ) {
                                    HandleData *d = static_cast<HandleData *>(h->data);

                                    Timer *self = static_cast<Timer *>(d->self);

                                    static_cast<Cont *>(d->continuation.get())->dispatch( self );
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

    class Async : public Handle<uv_async_t, Async> {
        public:
            typedef typename Handle<uv_async_t, Async>::handle_t handle_t;

        protected:
            void _init() {
                //No-op for uv_async_t
            }

        public:
            virtual inline std::shared_future<void> send_void() {
                return std::shared_future<void>();
            }
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
                arity =
                detail::function_traits<Functor>::arity - ( detail::ContinuationNeedsSelf<Functor, Async>::value )
            };

        public:
            inline void start( Functor f ) {
                this->internal_data.continuation = std::make_shared<Continuation>( f );

                uv_async_init( this->_loop, this->handle(), []( uv_async_t *h ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    auto *self = static_cast<AsyncDetail<Functor> *>(d->self);

                    std::lock_guard<std::mutex> lock( self->m );

                    Continuation *c = static_cast<Continuation *>(d->continuation.get());

                    c->dispatch( static_cast<Async *>(self));
                } );
            }

            /*
             * The enable_if is to generate slightly more appealing error messages when there are
             * incorrect number of arguments given. That way it fails here instead of deep into the details.
             * */
            template <typename... Args>
            inline typename std::enable_if<sizeof...( Args ) == arity, std::shared_future<result_type>>::type
            send( Args... args ) {
                std::lock_guard<std::mutex> lock( this->m );

                if( this->closing ) {
                    return std::async( std::launch::deferred, []() -> result_type {
                        throw ::uv::Exception( "async handle closed" );
                    } ).share();

                } else {
                    Continuation *c = static_cast<Continuation *>(this->internal_data.continuation.get());

                    auto ret = c->init( static_cast<Async *>(this), std::forward<Args>( args )... );

                    uv_async_send( this->handle());

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

            /*
             * This only exists when there are no arguments to send, so it can be called from Async* directly
             * */
            template <typename U = Functor, typename R = result_type,
                      typename = typename std::enable_if<
                          detail::function_traits<U>::arity == 1 && std::is_void<R>::value
                      >::type>
            inline std::shared_future<void> send_void() {
                return this->send();
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
                typedef detail::Continuation<Functor, Signal> Cont;

                this->internal_data.continuation = std::make_shared<Cont>( f );

                uv_signal_start( &_handle, []( uv_signal_t *h, int sn ) {
                    HandleData *d = static_cast<HandleData *>(h->data);

                    Signal *self = static_cast<Signal *>(d->self);

                    static_cast<Cont *>(d->continuation.get())->dispatch( self, sn );
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
        typedef HandleBase<typename D::handle_t, D> *argument_type;
        typedef std::size_t                         result_type;

        result_type operator()( argument_type const &h ) const {
            return reinterpret_cast<size_t>(h->handle());
        }
    };
}

#endif //UV_HANDLE_HPP
