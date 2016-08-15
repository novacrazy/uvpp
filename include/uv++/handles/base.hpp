//
// Created by Aaron on 8/1/2016.
//

#ifndef UV_BASE_HANDLE_HPP
#define UV_BASE_HANDLE_HPP

#include "../detail/data.hpp"

#include "../exception.hpp"

#include "../detail/handle.hpp"

#include <future>

namespace uv {
    template <typename H, typename D>
    struct HandleDataT : detail::UserData {
        /*
         * Shared pointers are used for these because of their type-erased deleters.
         * */

        //For primary continuation of callbacks
        std::shared_ptr<void> continuation;

        //Only used for close callbacks
        std::shared_ptr<void> close_continuation;

        /*
         * This is kept here to ensure a circular reference between the handle and the handle data
         * */
        std::shared_ptr<H> handle;

        /*
         * This is weak, because even if the handle data is still alive (via the above reference), it's okay
         * for the owning Handle object to be destroyed.
         * */
        std::weak_ptr<D> self;

        /*
         * These are just here to make continuation code cleaner at usage sites
         * */

        template <typename Cont>
        inline Cont *cont() {
            return static_cast<Cont *>(this->continuation.get());
        }

        template <typename Cont>
        inline Cont *close_cont() {
            return static_cast<Cont *>(this->close_continuation.get());
        }

        inline static void cleanup( H *h, std::weak_ptr<HandleDataT> *t ) {
            assert( h != nullptr );
            assert( t != nullptr );

            delete t;
            h->data = nullptr;
        }

        HandleDataT( std::shared_ptr<D> s, std::shared_ptr<H> h )
            : self( s ), handle( h ) {
        }
    };

    template <typename H, typename D>
    class HandleBase : public std::enable_shared_from_this<D>,
                       public detail::UserDataAccess<HandleDataT<H, D>, H>,
                       public detail::FromLoop {
        public:
            typedef typename detail::UserDataAccess<HandleDataT<H, D>, H>::handle_t   handle_t;
            typedef D                                                                 derived_type;
            typedef typename detail::UserDataAccess<HandleDataT<H, D>, H>::HandleData HandleData;

        protected:
            std::shared_ptr<HandleData> internal_data;
            std::shared_ptr<handle_t>   _handle;
            std::atomic_bool            closing;

            //Implemented in derived classes
            virtual void _init() = 0;

            virtual void _stop() = 0;

        public:
            inline void init( std::shared_ptr<Loop> l ) {
                this->_loop_init( l );

                this->internal_data = std::make_shared<HandleData>( std::static_pointer_cast<derived_type>( this->shared_from_this()), this->_handle );

                this->handle()->data = new std::weak_ptr<HandleData>( this->internal_data );

                this->_init();
            }

            void stop() {
                //TODO: Remove thread restriction
                assert( this->on_loop_thread());

                this->_stop();
            }

            virtual void start() {
                throw new ::uv::Exception( UV_ENOSYS );
            };
    };

    template <typename H, typename D>
    class Handle : public HandleBase<H, D> {
        public:
            typedef typename HandleBase<H, D>::handle_t     handle_t;
            typedef typename HandleBase<H, D>::derived_type derived_type;
            typedef typename HandleBase<H, D>::HandleData   HandleData;

            enum class handle_kind : std::underlying_type<uv_handle_type>::type {
                    UNKNOWN_HANDLE = 0,
#define XX( uc, lc ) uc = UV_##uc,
                    UV_HANDLE_TYPE_MAP( XX )
#undef XX
                    FILE,
                    HANDLE_TYPE_MAX
            };

        public:
            Handle() {
                this->_handle = std::make_shared<handle_t>();
            }

            inline const handle_t *handle() const noexcept {
                return this->_handle.get();
            }

            inline handle_t *handle() noexcept {
                return this->_handle.get();
            }

            inline bool is_active() const noexcept {
                return !this->closing && uv_is_active((uv_handle_t *)( this->handle())) != 0;
            }

            inline bool is_closing() const noexcept {
                return this->closing || uv_is_closing((uv_handle_t *)( this->handle())) != 0;
            }

            inline size_t size() noexcept {
                return uv_handle_size( this->handle()->type );
            }

            template <typename Functor>
            std::shared_future<void> close( Functor );

            inline handle_kind guess_handle_kind() const noexcept {
                return (handle_kind)uv_guess_handle( this->handle()->type );
            }

            std::string name() const noexcept {
                switch((handle_kind)this->handle()->type ) {
#define XX( uc, lc ) case handle_kind::uc: return #uc;
                    UV_HANDLE_TYPE_MAP( XX )
                    XX( FILE, file )
                    XX( HANDLE_TYPE_MAX, handle_type_max )
                    default:
                        return "UNKNOWN_HANDLE";
                }
            }

            std::string guess_handle_name() const noexcept {
                switch( this->guess_handle_kind()) {
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

        result_type operator()( argument_type const &h ) const noexcept {
            return reinterpret_cast<size_t>(h->handle());
        }
    };
}

#endif //UV_BASE_HANDLE_HPP
