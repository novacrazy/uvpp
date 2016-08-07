//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_BASE_HANDLE_HPP
#define UV_BASE_HANDLE_HPP

#include "../detail/base.hpp"

#include "../detail/data.hpp"

#include "../exception.hpp"

#include "../detail/handle.hpp"

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
                       public detail::UserDataAccess<HandleData, H>,
                       public detail::FromLoop {
        public:
            typedef typename detail::UserDataAccess<HandleData, H>::handle_t handle_t;
            typedef D                                                        derived_type;

        protected:
            HandleData internal_data;

            //Implemented in derived classes
            virtual void _init() = 0;

            virtual void _stop() = 0;

            //Can be called in subclasses
            void init( Loop *p, uv_loop_t *l ) {
                assert( p != nullptr );
                assert( l != nullptr );

                this->_parent_loop = p;
                this->_uv_loop        = l;

                this->handle()->data = &this->internal_data;

                this->_init();
            }

        public:
            inline HandleBase() : internal_data( this ) {}

            //Implemented in Loop.hpp to pass Loop::handle() to this->init(uv_loop_t*)
            inline void init( Loop * );

            std::thread::id loop_thread();

            void stop() {
                assert( std::this_thread::get_id() == this->loop_thread());

                this->_stop();
            }

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
                return !this->closing && uv_is_active((uv_handle_t *)( this->handle())) != 0;
            }

            inline bool is_closing() const {
                return this->closing || uv_is_closing((uv_handle_t *)( this->handle())) != 0;
            }

            inline size_t size() {
                return uv_handle_size( this->handle()->type );
            }

            template <typename Functor>
            std::shared_future<void> close( Functor );

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

    template <typename D>
    struct HandleHash {
        typedef HandleBase<typename D::handle_t, D> *argument_type;
        typedef std::size_t                         result_type;

        result_type operator()( argument_type const &h ) const {
            return reinterpret_cast<size_t>(h->handle());
        }
    };
}

#endif //UV_BASE_HANDLE_HPP
