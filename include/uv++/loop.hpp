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
#include <unordered_map>
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

            std::atomic_bool stopped, has_ran;

            typedef std::unordered_map<void *, std::weak_ptr<void>> weak_handle_map;
            typedef std::unordered_set<std::shared_ptr<void>>       handle_set;

            weak_handle_map                                           weak_handles;
            handle_set                                                handles;
            std::mutex                                                handle_mutex;

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
                }, true ); //Use weak reference since we're storing this one separately

                this->_fs = fs::Filesystem::make_filesystem( this->shared_from_this());
            }

            inline void _stop() {
                stopped = true;

                uv_stop( handle());
            }

        public:
            inline const handle_t *handle() const noexcept {
                return _handle.get();
            }

            inline handle_t *handle() noexcept {
                return _handle.get();
            }

        private:
            explicit inline Loop()
                : _loop_thread( std::this_thread::get_id())
#ifdef UV_USE_BOOST_LOCKFREE
                , task_queue( UV_LOCKFREE_QUEUE_SIZE )
#endif
            {}

        public:
            Loop( const Loop & ) = delete;

            //TODO: Figure out move semantics with atomic values
            //Loop( Loop && ) = delete;

            static inline std::shared_ptr<Loop> make_loop( handle_t *l = nullptr ) {
                auto loop = std::shared_ptr<Loop>( new Loop());

                if( l == nullptr ) {
                    loop->_handle = std::make_shared<handle_t>();

                } else {

                    struct no_deleter {
                        void operator()( handle_t *p ) const {
                        };
                    };

                    loop->_handle = std::shared_ptr<handle_t>( l, no_deleter());

                    loop->external = true;
                }

                loop->init( loop->shared_from_this());

                return loop;
            }

            inline std::shared_ptr<fs::Filesystem> fs() noexcept {
                return _fs;
            }

            inline int run( run_mode mode = RUN_DEFAULT ) noexcept {
                this->stopped = false;

                this->_loop_thread = std::this_thread::get_id();

                this->has_ran = true;

                return uv_run( this->handle(), (uv_run_mode)( mode ));
            }

            template <typename _Rep, typename _Period>
            void run_forever( const std::chrono::duration<_Rep, _Period> &delay, run_mode mode = RUN_DEFAULT ) noexcept {
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

            void cleanup() {
                std::lock_guard<std::mutex> lock( this->handle_mutex );

                //TODO: Convert this to explicit loop
                //std::remove_if( this->weak_handles.begin(), this->weak_handles.end(), []( const weak_handle_map::value_type &p ) -> bool {
                //    return p.second.expired();
                //} );
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
            std::shared_ptr<H> new_handle( bool requires_loop_thread, bool weak, Args... args ) {
                //The mutex prevents multiple initializations that affect the event loop at the same time
                std::lock_guard<std::mutex> lock( this->handle_mutex );

                if( this->has_ran && requires_loop_thread && !this->on_loop_thread()) {
                    /*
                     * If the new_handle request is not on the loop thread, block until everything is initialized there
                     *
                     * Not the best solution, but it just has to wait it's turn I guess. **shrugs**
                     * */
                    return this->schedule( []( std::shared_ptr<Loop> self, bool inner_weak, Args... inner_args ) {
                        assert( self->loop_thread() == std::this_thread::get_id());

                        return self->new_handle<H>( false, inner_weak, std::forward<Args>( inner_args )... );
                    }, weak, std::forward<Args>( args )... ).get();

                } else {
                    std::shared_ptr<H> p = std::make_shared<H>();

                    if( weak ) {
                        auto it_inserted = weak_handles.emplace( weak_handle_map::value_type{ p.get(), std::weak_ptr<void>( p ) } );

                        //There should be no collisions for new pointers, but yeah
                        assert( it_inserted.second );

                    } else {
                        auto it_inserted = handles.insert( p );

                        //There should be no collisions for new pointers, but yeah
                        assert( it_inserted.second );
                    }

                    p->init( this->shared_from_this());

                    p->start( std::forward<Args>( args )... );

                    return p;
                }
            }

        public:
            template <typename Functor>
            inline std::shared_ptr<Idle> idle( Functor f ) {
                return new_handle<Idle>( true, false, f );
            }

            template <typename Functor>
            inline std::shared_ptr<Prepare> prepare( Functor f ) {
                return new_handle<Prepare>( true, false, f );
            }

            template <typename Functor>
            inline std::shared_ptr<Check> check( Functor f ) {
                return new_handle<Check>( true, false, f );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> timer( Functor f,
                                                 const std::chrono::duration<_Rep, _Period> &timeout,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat =
                                                 std::chrono::duration<_Rep2, _Period2>(
                                                     std::chrono::duration_values<_Rep2>::zero()),
                                                 bool weak = false ) {
                return new_handle<Timer>( true, weak, f, timeout, repeat );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> repeat( Functor f,
                                                  const std::chrono::duration<_Rep, _Period> &repeat,
                                                  const std::chrono::duration<_Rep2, _Period2> &timeout =
                                                  std::chrono::duration<_Rep2, _Period2>(
                                                      std::chrono::duration_values<_Rep2>::zero()),
                                                  bool weak = false ) {
                return this->timer( f, timeout, repeat, weak );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2, typename _Period2>
            inline std::shared_ptr<Timer> timer( const std::chrono::duration<_Rep, _Period> &timeout,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat,
                                                 Functor f, bool weak = false ) {
                return this->timer( f, timeout, repeat, weak );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> timer( const std::chrono::duration<_Rep, _Period> &timeout,
                                                 Functor f,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat =
                                                 std::chrono::duration<_Rep2, _Period2>(
                                                     std::chrono::duration_values<_Rep2>::zero()),
                                                 bool weak = false ) {
                return this->timer( f, timeout, repeat, weak );
            }

            template <typename Functor,
                      typename _Rep, typename _Period,
                      typename _Rep2 = uint64_t, typename _Period2 = std::milli>
            inline std::shared_ptr<Timer> repeat( const std::chrono::duration<_Rep, _Period> &repeat,
                                                  Functor f,
                                                  const std::chrono::duration<_Rep2, _Period2> &timeout =
                                                  std::chrono::duration<_Rep2, _Period2>(
                                                      std::chrono::duration_values<_Rep2>::zero()),
                                                  bool weak = false ) {
                return this->timer( f, timeout, repeat, weak );
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
            inline std::shared_ptr<AsyncDetail<Functor>> async( Functor f, bool weak = false ) {
                return new_handle<AsyncDetail<Functor>>( true, weak, f );
            }

            template <typename Functor>
            inline std::shared_ptr<Signal> signal( int signal, Functor f ) {
                return new_handle<Signal>( true, false, signal, f );
            }

            template <typename Functor, typename... Args>
            UV_DECLTYPE_AUTO schedule( Functor f, Args... args ) {
                typedef detail::AsyncContinuation<Functor, Loop> Cont;

                Cont *c = new Cont( f );

                auto ret = c->init( this->shared_from_this(), std::forward<Args>( args )... );

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

            inline std::shared_ptr<Work> work( bool weak = false ) {
                //Work is special since it doesn't initialize on the loop thread
                return new_handle<Work>( false, weak );
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

    namespace detail {
        inline uv_loop_t *FromLoop::loop_handle() {
            return this->loop()->handle();
        }

        inline std::thread::id FromLoop::loop_thread() const {
            return this->loop()->_loop_thread;
        };

        inline bool FromLoop::on_loop_thread() const {
            return this->loop_thread() == std::this_thread::get_id();
        }

        struct DefaultLoop : LazyStatic<std::shared_ptr<Loop>> {
            std::shared_ptr<Loop> init() {
                return Loop::make_loop( uv_default_loop());
            }
        };

        static DefaultLoop default_loop;
    }

    inline std::shared_ptr<Loop> default_loop() {
        return detail::default_loop;
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

            auto ret = c->init( this->shared_from_this());

            this->internal_data->close_continuation = c;

            auto cb = []( uv_handle_t *h ) {
                std::weak_ptr<HandleData> *d = static_cast<std::weak_ptr<HandleData> *>(h->data);

                if( d != nullptr ) {
                    if( auto data = d->lock()) {
                        if( auto self = data->self.lock()) {
                            data->template close_cont<Cont>()->dispatch();

                            data->close_continuation.reset();
                        }

                    } else {
                        HandleData::cleanup( reinterpret_cast<H *>( h ), d );
                    }
                }
            };

            if( this->on_loop_thread()) {
                uv_close((uv_handle_t *)this->handle(), cb );

            } else {
                this->loop()->schedule( [this, cb] {
                    uv_close((uv_handle_t *)this->handle(), cb );
                } );
            }

            return ret;
        }
    }

    template <typename... Args>
    inline UV_DECLTYPE_AUTO schedule( std::shared_ptr<Loop> l, Args... args ) {
        return l->schedule( std::forward<Args>( args )... );
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
