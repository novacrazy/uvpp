//
// Created by Aaron on 7/24/2016.
//

#ifndef UV_LOOP_HPP
#define UV_LOOP_HPP

#include "base.hpp"
#include "type_traits.hpp"
#include "fwd.hpp"

#include "handle.hpp"
#include "fs.hpp"

#include <future>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <ostream>
#include <cstdio>
#include <iostream>
#include <iomanip>

namespace uv {
    class Loop : public HandleBase<uv_loop_t> {
        public:
            typedef typename HandleBase<uv_loop_t>::handle_t handle_t;

            template <typename H>
            friend
            class Handle;

            friend class Filesystem;

        private:
            uv_loop_t loop;

            Filesystem _fs;

            std::atomic_bool loop_stopped;

            typedef std::unordered_set<std::shared_ptr<void>> handle_set_type;
            handle_set_type                                   handle_set;

        protected:
            inline void _init( uv_loop_t *l ) {
                assert( l != nullptr );

                uv_loop_init( l );
            }

        public:
            enum class uv_option : std::underlying_type<uv_loop_option>::type {
                    BLOCK_SIGNAL = UV_LOOP_BLOCK_SIGNAL
            };

            enum class uvp_option {
                    WAIT_ON_CLOSE
            };

            inline const handle_t *handle() const {
                return &loop;
            }

            inline handle_t *handle() {
                return &loop;
            }

            inline Loop() : _fs( this ) {
                this->_initData();
                this->_init( this->handle());
            }

            inline Filesystem *fs() {
                return &_fs;
            }

            inline int run( uv_run_mode mode = UV_RUN_DEFAULT ) {
                loop_stopped = false;

                return uv_run( &loop, mode );
            }

            template <typename _Rep, typename _Period>
            inline void run_forever( const std::chrono::duration<_Rep, _Period> &delay ) {
                loop_stopped = false;

                while( !loop_stopped ) {
                    if( !this->run()) {
                        std::this_thread::sleep_for( delay );
                    }
                }
            }

            inline void run_forever() {
                using namespace std::chrono_literals;

                this->run_forever( 1ms );
            }

            inline void start() {
                this->run_forever();
            }

            inline void stop() {
                loop_stopped = true;

                uv_stop( &loop );
            }

            template <typename... Args>
            typename std::enable_if<all_type<uv_loop_option, Args...>::value, Loop &>::type
            inline configure( Args &&... args ) {
                auto res = uv_loop_configure( &loop, std::forward<Args>( args )... );

                if( res != 0 ) {
                    throw Exception( res );
                }

                return *this;
            }

            inline static size_t size() {
                return uv_loop_size();
            }

            inline int backend_fs() const {
                return uv_backend_fd( &loop );
            }

            inline int backend_timeout() const {
                return uv_backend_timeout( &loop );
            }

            inline uint64_t now() const {
                return uv_now( &loop );
            }

            inline void update_time() {
                uv_update_time( &loop );
            }

            //returns true on closed
            inline bool try_close( int *resptr = nullptr ) {
                int res = uv_loop_close( &loop );

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

            ~Loop() throw( Exception ) {
                for( std::shared_ptr<void> x : handle_set ) {
                    static_cast<Handle<uv_handle_t> *>(x.get())->stop();
                }

                this->stop();
                this->close();
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
            template <typename Functor>
            inline std::shared_ptr<Idle> idle( Functor f ) {
                return new_handle<Idle>( f );
            }

            template <typename Functor>
            inline std::shared_ptr<Prepare> prepare( Functor f ) {
                return new_handle<Prepare>( f );
            }

            template <typename Functor>
            inline std::shared_ptr<Check> check( Functor f ) {
                return new_handle<Check>( f );
            }

            template <typename Functor, typename _Rep, typename _Period, typename _Rep2, typename _Period2>
            inline std::shared_ptr<Timer> timer( Functor f,
                                                 const std::chrono::duration<_Rep, _Period> &timeout,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat = std::chrono::duration_values<_Rep2>::zero()) {
                return new_handle<Timer>( f, timeout, repeat );
            }

            template <typename Functor, typename _Rep, typename _Period, typename _Rep2, typename _Period2>
            inline std::shared_ptr<Timer> timer( const std::chrono::duration<_Rep, _Period> &timeout,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat,
                                                 Functor f ) {
                return this->timer( f, timeout, repeat );
            }

            template <typename Functor, typename _Rep, typename _Period, typename _Rep2, typename _Period2>
            inline std::shared_ptr<Timer> timer( const std::chrono::duration<_Rep, _Period> &timeout,
                                                 Functor f,
                                                 const std::chrono::duration<_Rep2, _Period2> &repeat = std::chrono::duration_values<_Rep2>::zero()) {
                return this->timer( f, timeout, repeat );
            }

            template <typename Functor>
            inline std::shared_ptr<Async> async( Functor f ) {
                return new_handle<Async>( f );
            }

            /*
             * This is such a mess, but that's what I get for mixing C and C++
             *
             * Most of it is partially copied from src/uv-common.(h|c) and src/queue.h
             * */
            template <typename _Char = char>
            void print_handles( bool only_active = false, std::basic_ostream<_Char> &out = std::cout ) const {
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
                const uv_loop_t   *loop = this->handle();

                UV_QUEUE_FOREACH( q, &loop->handle_queue ) {
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

    template <typename H>
    inline void Handle<H>::init( Loop *l ) {
        assert( l != nullptr );

        this->_initData();
        this->loop = &l->loop;
        this->_init();
    }

    void Filesystem::init( Loop *l ) {
        assert( l != nullptr );

        this->loop = &l->loop;
    }
}

#ifdef UV_OVERLOAD_OSTREAM

template <typename _Char>
std::basic_ostream<_Char> &operator<<( std::basic_ostream<_Char> &out, const uv::Loop &loop ) {
    loop.print_handles<_Char>( false, out );

    return out;
}

#endif

#endif //UV_LOOP_HPP
