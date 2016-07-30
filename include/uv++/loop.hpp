//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_LOOP_HPP
#define UV_LOOP_HPP

#include "fwd.hpp"

#include "exception.hpp"

#include "detail/type_traits.hpp"
#include "detail/utils.hpp"

#include "handle.hpp"
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
    class Loop final : public HandleBase<uv_loop_t> {
        public:
            typedef typename HandleBase<uv_loop_t>::handle_t handle_t;

            template <typename H, typename D>
            friend
            class Handle;

            friend class Filesystem;

            enum run_mode : std::underlying_type<uv_run_mode>::type {
                RUN_DEFAULT = UV_RUN_DEFAULT,
                RUN_ONCE    = UV_RUN_ONCE,
                RUN_NOWAIT  = UV_RUN_NOWAIT
            };

        private:
            bool external;

            Filesystem _fs;

            std::atomic_bool stopped;

            typedef std::unordered_set<std::shared_ptr<void>>                       handle_set_type;
            handle_set_type                                                         handle_set;

            typedef detail::TrivialPair<uv_handle_t *, void ( * )( uv_handle_t * )> close_args;

#ifdef UV_USE_BOOST_LOCKFREE
            boost::lockfree::queue<close_args> close_queue;
#else
            std::deque<close_args>   close_queue;
            std::mutex               close_mutex;
#endif

            std::shared_ptr<Async<>> close_async;
            std::thread::id          loop_thread;

        protected:
            inline void _init() {
                if( !this->external ) {
                    uv_loop_init( this->handle());
                }

                this->close_async = this->async( [this]( auto &a ) {
#ifdef UV_USE_BOOST_LOCKFREE
                    this->close_queue.consume_all( []( close_args &c ) {
                        uv_close( c.first, c.second );
                    } );
#else
                    std::lock_guard<std::mutex> lock( this->close_mutex );

                    for( close_args &c : this->close_queue ) {
                        uv_close( c.first, c.second );
                    }

                    this->close_queue.clear();
#endif
                    this->update_time();
                } );
            }

        public:
            enum class uv_option : std::underlying_type<uv_loop_option>::type {
                    BLOCK_SIGNAL = UV_LOOP_BLOCK_SIGNAL
            };

            enum class uvp_option {
                    WAIT_ON_CLOSE
            };

            inline const handle_t *handle() const {
                return _loop;
            }

            inline handle_t *handle() {
                return _loop;
            }

            inline Loop()
                : _fs( this ), external( false )
#ifdef UV_USE_BOOST_LOCKFREE
                , close_queue( 128 )
#endif
            {
                this->init( this, new handle_t );
            }

            explicit inline Loop( handle_t *l )
                : _fs( this ), external( true )
#ifdef UV_USE_BOOST_LOCKFREE
                , close_queue( 128 )
#endif
            {
                this->init( this, l );
            }

            inline Filesystem *fs() {
                return &_fs;
            }

            inline int run( run_mode mode = RUN_DEFAULT ) {
                this->stopped = false;

                this->loop_thread = std::this_thread::get_id();

                return uv_run( handle(), (uv_run_mode)( mode ));
            }

            template <typename _Rep, typename _Period>
            inline void run_forever( const std::chrono::duration<_Rep, _Period> &delay, run_mode mode = RUN_DEFAULT ) {
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
                    if( !this->run( mode ) || mode == RUN_NOWAIT ) {
                        std::this_thread::sleep_for( delay );
                    }
                }
#endif
            }

            inline void run_forever( run_mode mode = RUN_DEFAULT ) {
                using namespace std::chrono_literals;

                this->run_forever( UV_DEFAULT_LOOP_SLEEP, mode );
            }

            inline void start( run_mode mode = RUN_DEFAULT ) {
                this->run_forever( mode );
            }

            inline void stop() {
                stopped = true;

                uv_stop( handle());
            }

            template <typename... Args>
            typename std::enable_if<detail::all_type<uv_loop_option, Args...>::value, Loop &>::type
            inline configure( Args &&... args ) {
                auto res = uv_loop_configure( handle(), std::forward<Args>( args )... );

                if( res != 0 ) {
                    throw Exception( res );
                }

                return *this;
            }

            inline static size_t size() {
                return uv_loop_size();
            }

            inline int backend_fs() const {
                return uv_backend_fd( handle());
            }

            inline int backend_timeout() const {
                return uv_backend_timeout( handle());
            }

            inline uint64_t now() const {
                return uv_now( handle());
            }

            inline void update_time() {
                uv_update_time( handle());
            }

            //returns true on closed
            inline bool try_close( int *resptr = nullptr ) {
                int res = uv_loop_close( handle());

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
                for( std::shared_ptr<void> x : handle_set ) {
                    static_cast<HandleBase<uv_handle_t> *>(x.get())->stop();
                }

                if( !this->external ) {
                    this->stop();

                    delete handle();
                }
            }

        protected:
            template <typename H, typename... Args>
            inline std::shared_ptr<H> new_handle( Args &&... args ) {
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
                return new_handle<Idle>( std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline std::shared_ptr<Prepare> prepare( Args &&... args ) {
                return new_handle<Prepare>( std::forward<Args>( args )... );
            }

            template <typename... Args>
            inline std::shared_ptr<Check> check( Args &&... args ) {
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

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> repeat( Functor f,
                                                  const std::chrono::duration<_Rep, _Period> &repeat,
                                                  const std::chrono::duration<_Rep2, _Period2> &timeout =
                                                  std::chrono::duration<_Rep2, _Period2>(
                                                      std::chrono::duration_values<_Rep2>::zero())) {
                return new_handle<Timer>( f, timeout, repeat );
            }

            template <typename P = void, typename R = void, typename... Args>
            inline std::shared_ptr<Async<P, R>> async( Args &&... args ) {
                return new_handle<Async<P, R>>( std::forward<Args>( args )... );
            };

            template <typename... Args>
            inline std::shared_ptr<Signal> signal( Args &&... args ) {
                return new_handle<Signal>( std::forward<Args>( args )... );
            }

            /*
             * This is such a mess, but that's what I get for mixing C and C++
             *
             * Most of it is partially copied from src/uv-common.(h|c) and src/queue.h
             * */
            template <typename _Char = char>
            void print_handles( std::basic_ostream<_Char> &out, bool only_active = false ) const {
                typedef void *UV_QUEUE[2];
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

    namespace detail {
        static std::shared_ptr<::uv::Loop> default_loop_ptr;
    }

    std::shared_ptr<Loop> default_loop() {
        using namespace ::uv::detail;

        if( !default_loop_ptr ) {
            default_loop_ptr = std::make_shared<Loop>( uv_default_loop());
        }

        return default_loop_ptr;
    }

    template <typename H>
    inline void HandleBase<H>::init( Loop *l ) {
        assert( l != nullptr );

        this->init( l, l->handle());
    }

    namespace detail {
        template <typename Functor>
        struct CloseHelperContinuation : public Continuation<Functor> {
            inline CloseHelperContinuation( Functor f )
                : Continuation<Functor>( f ) {
            }

            std::promise<void> p;
        };
    }

    template <typename H, typename D>
    template <typename Functor>
    std::pair<std::future<void>, std::shared_ptr<std::shared_future<void>>>
    Handle<H, D>::close( Functor f ) {
        typedef detail::CloseHelperContinuation<Functor> Cont;

        auto current_thread = std::this_thread::get_id();
        auto loop_thread    = this->loop()->loop_thread;

        auto c = std::make_shared<Cont>( f );

        this->internal_data.secondary_continuation = c;

        auto queue_data = Loop::close_args{ (uv_handle_t *)( this->handle()), []( uv_handle_t *h ) {
            HandleData *d = static_cast<HandleData *>(h->data);

            typename Handle<H, D>::derived_type *self = static_cast<typename Handle<H, D>::derived_type *>(d->self);

            Cont *c2 = static_cast<Cont *>(d->secondary_continuation.get());

            try {
                c2->f( *self );

                c2->p.set_value();

            } catch( ... ) {
                c2->p.set_exception( std::current_exception());
            }

            d->secondary_continuation.reset();
        }};

        if( current_thread == loop_thread ) {
            uv_close( queue_data.first, queue_data.second );

            return std::make_pair( c->p.get_future(), nullptr );

        } else {
#ifdef UV_USE_BOOST_LOCKFREE
            this->loop()->close_queue.push( queue_data );
#else
            std::lock_guard<std::mutex> lock( this->loop()->close_mutex );

            this->loop()->close_queue.push_back( queue_data );
#endif
            return std::make_pair( c->p.get_future(),
                                   std::make_shared<std::shared_future<void>>( this->loop()->close_async->send()));
        }
    }

    void Filesystem::init( Loop *l ) {
        assert( l != nullptr );

        this->_loop = l->handle();
    }
}

#ifdef UV_OVERLOAD_OSTREAM

template <typename _Char>
std::basic_ostream<_Char> &operator<<( std::basic_ostream<_Char> &out, const uv::Loop &loop ) {
    loop.print_handles<_Char>( out, false );

    return out;
}

#endif

#endif //UV_LOOP_HPP
