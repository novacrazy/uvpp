//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_OS_HPP
#define UV_OS_HPP

#include "defines.hpp"
#include "exception.hpp"
#include "fwd.hpp"

#include <vector>
#include <array>
#include <algorithm>

namespace uv {
    namespace os {
        //uv_rusage_t is just all primitives, so it can be copied directly.
        using rusage_t = uv_rusage_t;

        /*
         * uv_cpu_info_t and uv_interface_address_t both allocate memory which needs to be freed by libuv,
         * so we just copy all the primitives and put the allocated c-strings into std::strings
         * */
        struct cpu_info_t {
            std::string                   model;
            int                           speed;
            uv_cpu_info_t::uv_cpu_times_s cpu_times;

            cpu_info_t( const uv_cpu_info_t &c )
                : model( c.model ),
                  speed( c.speed ),
                  cpu_times( c.cpu_times ) {
            }
        };

        /*
         * The choice to use std::array was just before iterators are nice. It doesn't really change anything.
         * */
        struct interface_address_t {
            std::string         name;
            std::array<char, 6> phys_addr;
            int                 is_internal;

            //Using decltype allows it to copy an anonymous union
            decltype( uv_interface_address_t().address ) address;
            decltype( uv_interface_address_t().netmask ) netmask;

            interface_address_t( const uv_interface_address_t &a )
                : name( a.name ),
                  is_internal( a.is_internal ),
                  address( a.address ),
                  netmask( a.netmask ) {
                std::copy( &a.phys_addr[0], &a.phys_addr[phys_addr.size()], phys_addr.begin());
            }

            inline std::string ipv4_address() const {
                //255.255.255.255
                char buffer[15] = { 0 };

                uv_ip4_name( &this->address.address4, buffer, sizeof( buffer ));

                return std::string( buffer );
            }

            inline bool is_ipv4_address() const {
                return this->address.address4.sin_family == AF_INET;
            }

            inline std::string ipv6_address() const {
                /*
                 * 0000.0000.0000.0000.0000.0000.0000.0000
                 *
                 * The buffer is 45 characters just in case libuv supports IPv4-mapped IPv6 addresses.
                 * See here: http://stackoverflow.com/a/1076755
                 * */
                char buffer[45] = { 0 };

                uv_ip6_name( &this->address.address6, buffer, sizeof( buffer ));

                return std::string( buffer );
            }

            inline bool is_ipv6_address() const {
                return this->address.address4.sin_family == AF_INET6;
            }

            inline std::string ip_address() const {
                return this->is_ipv4_address() ? this->ipv4_address() : this->ipv6_address();
            }
        };

        inline char **setup_args( int argc, char **argv ) {
            return uv_setup_args( argc, argv );
        }

        std::string process_title() {
            static size_t buffer_size = 16; //Good starting size, will statically increase if needed

            int res = 0;

            while( true ) {
                char *buffer = new char[buffer_size + 1];

                res = uv_get_process_title( buffer, buffer_size + 1 );

                if( res == UV_EINVAL ) {
                    /*
                     * If we received UV_EINVAL, that means either the buffer was null or the buffer_size was
                     * incorrect, so double check it here and delete if necessary
                     * */
                    if( buffer != nullptr ) {
                        delete[] buffer;
                    }

                    throw ::uv::Exception( res );

                } else if( res == UV_ENOBUFS ) {
                    buffer_size *= 2;

                    delete[] buffer;

                } else {
                    std::string ret( buffer );

                    delete[] buffer;

                    return std::move( ret );
                }
            }
        }

        void process_title( const std::string &title ) {
            int res = uv_set_process_title( title.c_str());

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }
        }

        size_t resident_set_memory() {
            size_t rss;

            int res = uv_resident_set_memory( &rss );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            return rss;
        }

        double uptime() {
            double u;

            int res = uv_uptime( &u );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            return u;
        }

        rusage_t rusage() {
            rusage_t u;

            int res = uv_getrusage( &u );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            return std::move( u );
        }

        std::vector<cpu_info_t> cpu_info() {
            uv_cpu_info_t *cpus;
            int           count;

            int res = uv_cpu_info( &cpus, &count );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            std::vector<cpu_info_t> c( &cpus[0], &cpus[count] );

            uv_free_cpu_info( cpus, count );

            return std::move( c );
        }

        std::vector<interface_address_t> interface_addresses() {
            uv_interface_address_t *addresses;
            int                    count;

            int res = uv_interface_addresses( &addresses, &count );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            std::vector<interface_address_t> a( &addresses[0], &addresses[count] );

            uv_free_interface_addresses( addresses, count );

            return std::move( a );
        }

    }
}

#endif //UV_OS_HPP
