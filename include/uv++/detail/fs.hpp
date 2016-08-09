//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_FS_DETAIL_HPP
#define UV_FS_DETAIL_HPP

#include "../fwd.hpp"

#include <string>
#include <chrono>

namespace uv {
    namespace fs {
        class Stat : public uv_stat_t {
            protected:
                //This is so only the Filesystem can create a Stat object directly from libuv
                Stat( const uv_stat_t &s )
                    : uv_stat_t( s ) {}

                friend class Filesystem;

            public:
                typedef std::chrono::system_clock::time_point time_point;

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

                inline bool is_link() const noexcept {
#ifdef S_ISLNK
                    return S_ISLNK( this->st_mode );
#else
                    return ( this->st_mode & S_IFMT ) == S_IFLNK; //libuv helpfully defines this for us
#endif
                }

                /*
                 * S_IFSOCK value taken from http://man7.org/linux/man-pages/man2/stat.2.html
                 * */
                inline bool is_socket() const noexcept {
#ifdef S_ISSOCK
                    return S_ISSOCK( this->st_mode );
#else
# ifndef S_IFSOCK
#  define S_IFSOCK 0x222E0 //libuv does not provide this, though
# endif
                    return ( this->st_mode & S_IFMT ) == S_IFSOCK;
#endif
                }

                std::string permissions() const noexcept {
                    std::string buf( "drwxrwxrwx" );

                    buf[0] = "-d"[this->is_dir()];
                    buf[1] = "-r"[( this->st_mode & S_IRUSR ) != 0];
                    buf[2] = "-w"[( this->st_mode & S_IWUSR ) != 0];
                    buf[3] = "-x"[( this->st_mode & S_IXUSR ) != 0];
                    buf[4] = "-r"[( this->st_mode & S_IRGRP) != 0];
                    buf[5] = "-w"[( this->st_mode & S_IWGRP) != 0];
                    buf[6] = "-x"[( this->st_mode & S_IXGRP) != 0];
                    buf[7] = "-r"[( this->st_mode & S_IROTH) != 0];
                    buf[8] = "-w"[( this->st_mode & S_IWOTH) != 0];
                    buf[8] = "-x"[( this->st_mode & S_IXOTH) != 0];

                    return buf;
                }

                time_point atime() const noexcept {
                    using namespace std::chrono;

                    return time_point( duration_cast<system_clock::duration>(
                        seconds{ this->st_atim.tv_sec } + nanoseconds{ this->st_atim.tv_nsec }
                    ));
                }

                time_point mtime() const noexcept {
                    using namespace std::chrono;

                    return time_point( duration_cast<system_clock::duration>(
                        seconds{ this->st_mtim.tv_sec } + nanoseconds{ this->st_mtim.tv_nsec }
                    ));
                }

                time_point ctime() const noexcept {
                    using namespace std::chrono;

                    return time_point( duration_cast<system_clock::duration>(
                        seconds{ this->st_ctim.tv_sec } + nanoseconds{ this->st_ctim.tv_nsec }
                    ));
                }

                time_point birthtime() const noexcept {
                    using namespace std::chrono;

                    return time_point( duration_cast<system_clock::duration>(
                        seconds{ this->st_birthtim.tv_sec } + nanoseconds{ this->st_birthtim.tv_nsec }
                    ));
                }
        };
    }
}

#ifdef UV_OVERLOAD_OSTREAM

#include <iostream>
#include <iomanip>
#include <cmath>

#ifdef UV_USE_BOOST_FORMAT

#include <boost/format.hpp>

#else

#include <sstream>

#endif

template <typename _Char>
inline std::basic_ostream<_Char> &operator<<( std::basic_ostream<_Char> &out, const uv::fs::Stat &s ) noexcept {
    using namespace std;

    constexpr auto all_modes = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

    constexpr const char *dformat = "%Y-%m-%d %H:%M:%S";

    time_t atime = chrono::system_clock::to_time_t( s.atime()),
           mtime = chrono::system_clock::to_time_t( s.mtime()),
           ctime = chrono::system_clock::to_time_t( s.ctime()),
           btime = chrono::system_clock::to_time_t( s.birthtime());

    tm *atime_tm = gmtime( &atime ),
       *mtime_tm = gmtime( &mtime ),
       *ctime_tm = gmtime( &ctime ),
       *btime_tm = gmtime( &btime );

    bool   special = false;
    string type;

    switch( s.st_mode & S_IFMT ) {
        case S_IFSOCK:
            type = "socket";
            break;
        case S_IFLNK:
            type = "symbolic link";
            break;
        case S_IFREG:
            type = "regular file";
            break;
        case S_IFBLK:
            type    = "block device";
            special = true;
            break;
        case S_IFDIR:
            type = "directory";
            break;
        case S_IFCHR:
            type    = "character device";
            special = true;
            break;
        case S_IFIFO:
            type = "FIFO";
            break;
        default:
            type = "unknown";
    }

    auto dev = special ? s.st_rdev : s.st_dev;

#ifdef UV_USE_BOOST_FORMAT
    auto device = boost::basic_format<_Char>( "%xh/%dd" ) % dev % dev;
#else
    std::basic_stringstream<_Char> device;

    device << hex << dev << "h/" << dec << dev << 'd';
#endif

    auto device_str = device.str();

    size_t max_width = std::max( { (size_t)18, device_str.length() + 1, (size_t)std::log10( s.st_ino ) + 1 } );

    size_t perm_width = max_width > 16 ? max_width - 16 : 0;

    out << right << setw( 8 ) << "Size: " << left << setw( max_width ) << s.st_size
        << left << setw( 8 ) << "Blocks: " << left << setw( max_width ) << s.st_blocks
        << left << setw( 8 ) << "IO Blocks: " << s.st_blksize << "   " << type;

    out << endl;

    out << right << setw( 8 ) << "Device: " << left << setw( max_width ) << device_str
        << left << setw( 8 ) << "Inode: " << left << setw( max_width ) << s.st_ino
        << left << setw( 11 ) << "Links: " << s.st_nlink;

    out << endl;

    out << right << setw( 8 ) << "Access: (0" << oct << ( s.st_mode & all_modes ) << '/' << s.permissions() << left << setw( perm_width ) << ")"
        << left << setw( 8 ) << "Uid: " << left << setw( max_width ) << s.st_uid
        << left << setw( 11 ) << "Gid: " << s.st_gid;

    out << endl;

    out << right << setw( 8 ) << "Access: " << put_time( atime_tm, dformat ) << '.' << s.st_atim.tv_nsec << put_time( atime_tm, " %z" ) << endl;
    out << right << setw( 8 ) << "Modify: " << put_time( mtime_tm, dformat ) << '.' << s.st_mtim.tv_nsec << put_time( mtime_tm, " %z" ) << endl;
    out << right << setw( 8 ) << "Change: " << put_time( ctime_tm, dformat ) << '.' << s.st_ctim.tv_nsec << put_time( ctime_tm, " %z" ) << endl;
    out << right << setw( 8 ) << "Birth: " << put_time( btime_tm, dformat ) << '.' << s.st_birthtim.tv_nsec << put_time( btime_tm, " %z" ) << endl;

    return out;
}

#endif

#endif //UV_FS_DETAIL_HPP
