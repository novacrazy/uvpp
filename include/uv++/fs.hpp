//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_FS_HPP
#define UV_FS_HPP

#include "defines.hpp"
#include "fwd.hpp"

#include "detail/fs_detail.hpp"

#include <memory>
#include <future>

namespace uv {
    class File : public std::enable_shared_from_this<File> {
        protected:
            uv_loop_t *loop;

        public:
            File( uv_loop_t *l ) : loop( l ) {}

            friend class Filesystem;

            std::future<std::shared_ptr<File>> open( const std::string &path, int flags, int mode ) {

            }

            ~File() {

            }
    };

    class Filesystem {
        private:
            uv_loop_t *loop;

            void init( Loop * );

        public:
            Filesystem( Loop *l ) {
                this->init( l );
            }

            inline std::shared_ptr<::uv::File> File() {
                return std::make_shared<::uv::File>( this->loop );
            }

            template <typename... Args>
            inline std::future<std::shared_ptr<::uv::File>> open( Args &&... args ) {
                return this->File()->open( std::forward<Args>( args )... );
            };
    };
}

#endif //UV_FS_HPP
