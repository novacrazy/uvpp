//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_FS_HPP
#define UV_FS_HPP

#include "requests/fs.hpp"
#include "detail/fs.hpp"

namespace uv {
    namespace fs {
        template <typename T>
        class FSResult : public std::future<T> {
            protected:
                std::unique_ptr <FSRequest> _request;

                inline FSResult( std::future <T> &&f, std::unique_ptr <FSRequest> &&r ) noexcept
                    : std::future<T>( std::move( f )), _request( std::move( r )) {
                }

            public:
                friend class Filesystem;

                friend class FSRequest;

                FSResult( std::future <T> && ) = delete;

                FSResult( const std::future <T> & ) = delete;

                FSResult() = delete;

                inline FSResult( FSResult &&f ) noexcept
                    : FSResult( std::move( f.as_future()), std::move( f._request )) {
                }

                inline FSRequest *request() noexcept {
                    return this->_request.get();
                }

                inline std::future <T> &as_future() noexcept {
                    return *static_cast<std::future <T> *>(this);
                }

                //Just a simple shortcut to make cancellation easier
                inline std::shared_future<void> cancel() {
                    return this->_request->cancel();
                }
        };

        class Stat : public uv_stat_t {
            protected:
                //This is so only the Filesystem can create a Stat object directly from libuv
                Stat( const uv_stat_t &s )
                    : uv_stat_t( s ) {}

                friend class Filesystem;

            public:
                Stat( const Stat & ) = default;

                inline bool is_block() const noexcept {
                    return S_ISBLK( this->st_mode );
                }

                inline bool is_char() const noexcept {
                    return S_ISCHR( this->st_mode );
                }

                inline bool is_dir() const noexcept {
                    return S_ISDIR( this->st_mode );
                }

                inline bool is_fifo() const noexcept {
                    return S_ISFIFO( this->st_mode );
                }

                inline bool is_reg() const noexcept {
                    return S_ISREG( this->st_mode );
                }

                //Might not be available on all platforms, like windows
#ifdef S_ISLNK
                inline bool is_link() const noexcept {
                    return S_ISLNK( this->st_mode );
                }
#endif

                std::string permissions() const noexcept {
                    std::string buf( "drwxrwxrwx" );

                    buf[0] = "-d"[this->is_dir()];
                    buf[1] = "-r"[( this->st_mode & S_IRUSR ) != 0];
                    buf[2] = "-w"[( this->st_mode & S_IWUSR ) != 0];
                    buf[3] = "-x"[( this->st_mode & S_IXUSR ) != 0];
                    buf[4] = "-r"[( this->st_mode & S_IRGRP ) != 0];
                    buf[5] = "-w"[( this->st_mode & S_IWGRP ) != 0];
                    buf[6] = "-x"[( this->st_mode & S_IXGRP ) != 0];
                    buf[7] = "-r"[( this->st_mode & S_IROTH ) != 0];
                    buf[8] = "-w"[( this->st_mode & S_IWOTH ) != 0];
                    buf[8] = "-x"[( this->st_mode & S_IXOTH ) != 0];

                    return buf;
                }
        };

        class Filesystem : public std::enable_shared_from_this<Filesystem>,
                           public ::uv::detail::FromLoop {
            private:
                inline void init( Loop *l ) noexcept {
                    this->_loop_init( l );
                }

            public:
                Filesystem( Loop *l ) noexcept {
                    assert( l != nullptr );

                    this->init( l );
                }

                FSResult<Stat> stat( const std::string &path ) {
                    auto request = std::make_unique<FSRequest>();

                    request->init( this->loop());

                    auto result = util::then( request->promisify( uv_fs_stat, path.c_str()), []( uv_fs_t *req ) {
                        Stat a( req->statbuf );

                        uv_fs_req_cleanup( req );

                        return a;
                    } );

                    //FSResult takes ownership of request
                    return FSResult<Stat>( std::move( result ), std::move( request ));
                }
        };
    }
}

#endif //UV_FS_HPP
