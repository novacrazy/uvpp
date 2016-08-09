//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_LOOP_HPP
#define UV_LOOP_HPP

#include "handle.hpp"
#include "request.hpp"
#include "fs.hpp"

#include <thread>
#include <unordered_set>
#include <iomanip>

#ifndef UV_DEFAULT_LOOP_SLEEP
#define UV_DEFAULT_LOOP_SLEEP 1ms
#endif

#ifdef UV_USE_BOOST_LOCKFREE

#include <boost/lockfree/queue.hpp>

#else

#include <mutex>
#include <deque>

#endif

namespace uv {
    class Loop final : public HandleBase<uv_loop_t, Loop> {
        public:
            typedef typename HandleBase<uv_loop_t, Loop>::handle_t handle_t;

            template <typename H, typename D>
            friend
            class Handle;

            friend class detail::FromLoop;

            friend class fs::Filesystem;

            enum run_mode : std::underlying_type<uv_run_mode>::type {
                RUN_DEFAULT = UV_RUN_DEFAULT,
                RUN_ONCE    = UV_RUN_ONCE,
                RUN_NOWAIT  = UV_RUN_NOWAIT
            };

            enum class uv_option : std::underlying_type<uv_loop_option>::type {
                    BLOCK_SIGNAL = UV_LOOP_BLOCK_SIGNAL
            };

            enum class uvp_option {
                    WAIT_ON_CLOSE
            };

        private:
            bool external;

            std::shared_ptr<fs::Filesystem> _fs;

            std::atomic_bool stopped;

            typedef std::unordered_set<std::shared_ptr<void>>         handle_set_type;
            handle_set_type                                           handle_set;
            std::mutex                                                handle_set_mutex;

            typedef detail::TrivialPair<void *, void ( * )( void * )> scheduled_task;

#ifdef UV_USE_BOOST_LOCKFREE
            boost::lockfree::queue<scheduled_task> task_queue;
#else
            std::deque<scheduled_task> task_queue;
            std::mutex                 schedule_mutex;
#endif
            std::shared_ptr<Async> schedule_async;

        protected:
            std::thread::id _loop_thread;

            inline void _init() {
                if( !this->external ) {
                    uv_loop_init( this->handle());
                }

                this->schedule_async = this->async( [this] {
                    assert( std::this_thread::get_id() == this->loop_thread());

#ifdef UV_USE_BOOST_LOCKFREE
                    this->task_queue.consume_all( [this]( scheduled_task &task ) {
                        task.second( task.first );
                    } );
#else
                    {
                        std::lock_guard<std::mutex> lock( this->schedule_mutex );

                        for( scheduled_task &task : this->task_queue ) {
                            task.second( task.first );
                        }

                        this->task_queue.clear();
                    }
#endif
                    this->update_time();
                } );

                this->_fs = std::make_shared<fs::Filesystem>( this );
            }

            inline void _stop() {
                stopped = true;

                uv_stop( handle());
            }

        public:
            inline const handle_t *handle() const noexcept {
                return _uv_loop;
            }

            inline handle_t *handle() noexcept {
                return _uv_loop;
            }

            inline Loop()
                : external( false ), _loop_thread( std::this_thread::get_id())
#ifdef UV_USE_BOOST_LOCKFREE
                , task_queue( UV_LOCKFREE_QUEUE_SIZE )
#endif
            {
                this->init( this, new handle_t );
            }

            explicit inline Loop( handle_t *l )
                : external( true ), _loop_thread( std::this_thread::get_id())
#ifdef UV_USE_BOOST_LOCKFREE
                , task_queue( UV_LOCKFREE_QUEUE_SIZE )
#endif
            {
                this->init( this, l );
            }

            inline std::shared_ptr<fs::Filesystem> fs() noexcept {
                return _fs;
            }

            inline int run( run_mode mode = RUN_DEFAULT ) noexcept {
                this->stopped = false;

                this->_loop_thread = std::this_thread::get_id();

                return uv_run( handle(), (uv_run_mode)( mode ));
            }

            template <typename _Rep, typename _Period>
            void
            run_forever( const std::chrono::duration<_Rep, _Period> &delay, run_mode mode = RUN_DEFAULT ) noexcept {
                this->stopped = false;

#ifdef UV_DETRACT_LOOP
                typedef std::chrono::nanoseconds                       nano;
                typedef std::chrono::high_resolution_clock::time_point time_point;

                nano delay_ns = std::chrono::duration_cast<nano>( delay );
                nano detract  = std::chrono::duration_values<nano>::zero();
                nano diff;

                time_point pre, post;

                while( !this->stopped ) {
                    if(( !this->run( mode ) || mode == RUN_NOWAIT ) && detract < delay_ns ) {
                        pre = std::chrono::high_resolution_clock::now();

                        std::this_thread::sleep_for( delay_ns - detract );

                        post = std::chrono::high_resolution_clock::now();

                        diff = post - pre;

                        if( diff > delay_ns ) {
                            detract = diff - delay_ns;

                        } else {
                            detract = std::chrono::duration_values<nano>::zero();
                        }
                    }
                }
#else
                while( !this->stopped ) {
                    if( this->run( mode ) == 0 || mode == RUN_NOWAIT ) {
                        std::this_thread::sleep_for( delay );
                    }
                }
#endif
            }

            inline void run_forever( run_mode mode = RUN_DEFAULT ) noexcept {
                using namespace std::chrono_literals;

                this->run_forever( UV_DEFAULT_LOOP_SLEEP, mode );
            }

            inline void start( run_mode mode = RUN_DEFAULT ) noexcept {
                this->run_forever( mode );
            }

            template <typename... Args>
            typename std::enable_if<detail::all_type<uv_loop_option, Args...>::value, Loop &>::type
            configure( Args &&... args ) {
                assert( std::this_thread::get_id() == this->loop_thread());

                auto res = uv_loop_configure( this->handle(), std::forward<Args>( args )... );

                if( res != 0 ) {
                    throw Exception( res );
                }

                return *this;
            }

            inline static size_t size() noexcept {
                return uv_loop_size();
            }

            inline int backend_fd() const noexcept {
                assert( std::this_thread::get_id() == this->loop_thread());

                return uv_backend_fd( handle());
            }

            inline int backend_timeout() const noexcept {
                assert( std::this_thread::get_id() == this->loop_thread());

                return uv_backend_timeout( handle());
            }

            inline uint64_t now() const noexcept {
                return uv_now( handle());
            }

            inline void update_time() noexcept {
                uv_update_time( handle());
            }

            //returns true on closed
            inline bool try_close( int *resptr = nullptr ) noexcept {
                assert( std::this_thread::get_id() == this->loop_thread());

                int res = uv_loop_close( this->handle());

                if( resptr != nullptr ) {
                    *resptr = res;
                }

                return ( res == 0 );
            }

            //returns true on closed, throws otherwise
            inline bool close() {
                int res;

                if( !this->try_close( &res )) {
                    throw Exception( res );
                }

                return true;
            }

            ~Loop() {
                if( !this->external ) {
                    this->stop();
                    delete handle();
                }
            }

        protected:
            template <typename H, typename... Args>
            std::shared_ptr<H> new_handle( Args &&... args ) {
                std::lock_guard<std::mutex> lock( this->handle_set_mutex );

                std::shared_ptr<H> p = std::make_shared<H>();

                auto it_inserted = handle_set.insert( p );

                //On the really.... REALLY off chance there is a collision for a new pointer, just use the old one
                if( !it_inserted.second ) {
                    p = std::static_pointer_cast<H>( *it_inserted.first );
                }

                p->init( this );

                p->start( std::forward<Args>( args )... );

                return p;
            }

        public:
            template <typename... Args>
            inline std::shared_ptr<Idle> idle( Args &&... args ) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<Idle>( std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline std::shared_ptr<Prepare> prepare( Args &&... args ) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<Prepare>( std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline std::shared_ptr<Check> check( Args &&... args ) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<Check>( std::forward<Args>( args )... );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> timer( Functor f,
                                                 const std::chrono::duration<_Rep, _Period> &timeout,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat =
                                                 std::chrono::duration<_Rep2, _Period2>(
                                                     std::chrono::duration_values<_Rep2>::zero())) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<Timer>( f, timeout, repeat );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> repeat( Functor f,
                                                  const std::chrono::duration<_Rep, _Period> &repeat,
                                                  const std::chrono::duration<_Rep2, _Period2> &timeout =
                                                  std::chrono::duration<_Rep2, _Period2>(
                                                      std::chrono::duration_values<_Rep2>::zero())) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<Timer>( f, timeout, repeat );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2, typename _Period2>
            inline std::shared_ptr<Timer> timer( const std::chrono::duration<_Rep, _Period> &timeout,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat,
                                                 Functor f ) {
                return this->timer( f, timeout, repeat );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> timer( const std::chrono::duration<_Rep, _Period> &timeout,
                                                 Functor f,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat =
                                                 std::chrono::duration<_Rep2, _Period2>(
                                                     std::chrono::duration_values<_Rep2>::zero())) {
                return this->timer( f, timeout, repeat );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> repeat( const std::chrono::duration<_Rep, _Period> &repeat,
                                                  Functor f,
                                                  const std::chrono::duration<_Rep2, _Period2> &timeout =
                                                  std::chrono::duration<_Rep2, _Period2>(
                                                      std::chrono::duration_values<_Rep2>::zero())) {
                return this->timer( f, timeout, repeat );
            }

            template <typename... Args>
            inline std::shared_ptr<Timer> interval( Args... args ) {
                return this->repeat( std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline std::shared_ptr<Timer> timeout( Args... args ) {
                return this->timer( std::forward<Args>( args )... );
            }

            template <typename Functor>
            inline std::shared_ptr<AsyncDetail<Functor>> async( Functor f ) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<AsyncDetail<Functor>>( f );
            }

            template <typename... Args>
            inline std::shared_ptr<Signal> signal( Args &&... args ) {
                assert( std::this_thread::get_id() == this->loop_thread());

                return new_handle<Signal>( std::forward<Args>( args )... );
            }

            template <typename Functor, typename... Args>
            decltype( auto ) schedule( Functor f, Args... args ) {
                typedef detail::AsyncContinuation<Functor, Loop> Cont;

                Cont *c = new Cont( f );

                auto ret = c->init( this, std::forward<Args>( args )... );

                scheduled_task t{ c, []( void *vc ) {
                    Cont *sc = static_cast<Cont *>(vc);

                    sc->dispatch();

                    delete sc;
                }};

#ifdef UV_USE_BOOST_LOCKFREE
                this->task_queue.push( t );
#else
                {
                    //Only lock the duration of the push_back
                    std::lock_guard<std::mutex> lock( this->schedule_mutex );

                    this->task_queue.push_back( t );
                }
#endif
                this->schedule_async->send_void();

                return ret;
            }

            inline std::shared_ptr<Work> work() {
                return new_handle<Work>();
            };

            /*
             * This is such a mess, but that's what I get for mixing C and C++
             *
             * Most of it is partially copied from src/uv-common.(h|c) and src/queue.h
             * */
            template <typename _Char = char>
            void print_handles( std::basic_ostream<_Char> &out, bool only_active = false ) const noexcept {
                typedef void *UV_QUEUE[2];

                assert( std::this_thread::get_id() == this->loop_thread());
#ifndef _WIN32
                enum {
                  UV__HANDLE_INTERNAL = 0x8000,
                  UV__HANDLE_ACTIVE   = 0x4000,
                  UV__HANDLE_REF      = 0x2000,
                  UV__HANDLE_CLOSING  = 0 /* no-op on unix */
                };
#else
#define UV__HANDLE_INTERNAL  0x80
#define UV__HANDLE_ACTIVE    0x40
#define UV__HANDLE_REF       0x20
#define UV__HANDLE_CLOSING   0x01
#endif

#define UV_QUEUE_NEXT( q )                  (*(UV_QUEUE **) &((*(q))[0]))
#define UV_QUEUE_DATA( ptr, type, field )   ((type *) ((char *) (ptr) - offsetof(type, field)))
#define UV_QUEUE_FOREACH( q, h )            for ((q) = UV_QUEUE_NEXT(h); (q) != (h); (q) = UV_QUEUE_NEXT(q))

                const char        *type;
                UV_QUEUE          *q;
                uv_handle_t const *h;

                UV_QUEUE_FOREACH( q, &handle()->handle_queue ) {
                    h = UV_QUEUE_DATA( q, uv_handle_t, handle_queue );

                    if( !only_active || ( h->flags & UV__HANDLE_ACTIVE != 0 )) {

                        switch( h->type ) {
#define X( uc, lc ) case UV_##uc: type = #lc; break;
                            UV_HANDLE_TYPE_MAP( X )
#undef X
                            default:
                                type = "<unknown>";
                        }

                        out << "["
                            << "R-"[!( h->flags & UV__HANDLE_REF )]
                            << "A-"[!( h->flags & UV__HANDLE_ACTIVE )]
                            << "I-"[!( h->flags & UV__HANDLE_INTERNAL )]
                            << "] " << std::setw( 8 ) << std::left << type
                            << " 0x" << h
                            << std::endl;
                    }
                }

#undef UV_QUEUE_NEXT
#undef UV_QUEUE_DATA
#undef UV_QEUEU_FOREACH

#ifdef _WIN32
#undef UV__HANDLE_INTERNAL
#undef UV__HANDLE_ACTIVE
#undef UV__HANDLE_REF
#undef UV__HANDLE_CLOSING
#endif
            }
    };

    inline std::thread::id detail::FromLoop::loop_thread() const noexcept {
        return this->loop()->_loop_thread;
    };

    namespace detail {
        struct DefaultLoop : LazyStatic<std::shared_ptr<Loop>> {
            std::shared_ptr<Loop> init() {
                return std::make_shared<Loop>( uv_default_loop());
            }
        };

        static DefaultLoop default_loop;
    }

    inline std::shared_ptr<Loop> default_loop() {
        return detail::default_loop;
    }

    template <typename H, typename D>
    inline void HandleBase<H, D>::init( Loop *l ) {
        assert( l != nullptr );

        this->init( l, l->handle());
    }

    template <typename H, typename D>
    inline void Request<H, D>::init( Loop *l ) {
        assert( l != nullptr );

        this->init( l, l->handle());
    }

    //TODO: Replace this with something similar to the above init functions
    void fs::Filesystem::init( Loop *l ) {
        assert( l != nullptr );

        this->_uv_loop = l->handle();
    }

    template <typename H, typename D>
    template <typename Functor>
    std::shared_future<void> Handle<H, D>::close( Functor f ) {
        typedef detail::AsyncContinuation<Functor, D> Cont;

        /*
         * Using the atomic compare and exchange code below, this can atomically test whether closing is true, and if it
         * isn't, then set it to true for the upcoming close. It also ensures expect_closing is equal to closing before
         * the exchange.
         * */

        bool expect_closing = false;

        this->closing.compare_exchange_strong( expect_closing, true );

        if( expect_closing ) {
            return detail::make_exception_future<void>( ::uv::Exception( "handle already closing or closed" ));

        } else {
            auto c = std::make_shared<Cont>( f );

            this->internal_data.close_continuation = c;

            auto ret = c->init( this );

            auto cb = []( uv_handle_t *h ) {
                HandleData *d = static_cast<HandleData *>(h->data);

                auto sc = std::static_pointer_cast<Cont>( d->close_continuation );

                sc->dispatch();

                d->close_continuation.reset();
            };

            if( std::this_thread::get_id() != this->loop()->loop_thread()) {
                this->loop()->schedule( [this, cb] {
                    uv_close((uv_handle_t *)this->handle(), cb );
                } );

            } else {
                uv_close((uv_handle_t *)this->handle(), cb );
            }

            return ret;
        }
    }

    namespace detail {
        template <typename... Args>
        inline decltype( auto ) schedule( Loop *l, Args... args ) {
            return l->schedule( std::forward<Args>( args )... );
        }
    }
}

#ifdef UV_OVERLOAD_OSTREAM

template <typename _Char>
inline std::basic_ostream<_Char> &operator<<( std::basic_ostream<_Char> &out, const uv::Loop &loop ) noexcept {
    loop.print_handles<_Char>( out, false );

    return out;
}

#endif

#endif //UV_LOOP_HPP
