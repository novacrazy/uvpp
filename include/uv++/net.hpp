//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_NET_HPP
#define UV_NET_HPP

#include "fwd.hpp"

#include "exception.hpp"

#include <vector>
#include <array>
#include <algorithm>

namespace uv {
    namespace net {

        //Using decltype allows it to copy an anonymous union
        typedef decltype( uv_interface_address_t().address ) address_t;
        typedef decltype( uv_interface_address_t().netmask ) netmask_t;

        /*
         * The choice to use std::array was just before iterators are nice. It doesn't really change anything.
         * */
        struct interface_t {
            std::string         name;
            std::array<char, 6> phys_addr;
            int                 is_internal;

            address_t address;
            netmask_t netmask;

            interface_t( const uv_interface_address_t &a )
                : name( a.name ),
                  is_internal( a.is_internal ),
                  address( a.address ),
                  netmask( a.netmask ) {
                std::copy( &a.phys_addr[0], &a.phys_addr[phys_addr.size()], phys_addr.begin());
            }

            inline std::string ipv4_address() const {
                char buffer[INET_ADDRSTRLEN] = { 0 };

                uv_ip4_name( &this->address.address4, buffer, sizeof( buffer ));

                return std::string( buffer );
            }

            inline bool is_ipv4_address() const {
                return this->address.address4.sin_family == AF_INET;
            }

            inline std::string ipv6_address() const {
                char buffer[INET6_ADDRSTRLEN] = { 0 };

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

        inline void set_ip4_addr( sockaddr_in *addr, const std::string &ip, int port ) {
            assert( addr != nullptr );

            int res = uv_ip4_addr( ip.c_str(), port, addr );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }
        }

        inline sockaddr_in ip4_addr( const std::string &ip, int port ) {
            sockaddr_in tmp;
            set_ip4_addr( &tmp, ip, port );
            return tmp;
        }

        inline void set_ip6_addr( sockaddr_in6 *addr, const std::string &ip, int port ) {
            assert( addr != nullptr );

            int res = uv_ip6_addr( ip.c_str(), port, addr );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }
        }

        inline sockaddr_in6 ip6_addr( const std::string &ip, int port ) {
            sockaddr_in6 tmp;
            set_ip6_addr( &tmp, ip, port );
            return tmp;
        }

        inline void set_ip_addr( address_t *addr, const std::string &ip, int port ) {
            assert( addr != nullptr );

            auto af = ip.find( ':' ) == std::string::npos ? AF_INET : AF_INET6;

            if( af == AF_INET ) {
                set_ip4_addr( &addr->address4, ip, port );

            } else {
                set_ip6_addr( &addr->address6, ip, port );
            }
        }

        inline address_t ip_addr( const std::string &ip, int port ) {
            address_t tmp;
            set_ip_addr( &tmp, ip, port );
            return tmp;
        }

        std::string ntop( const address_t &address ) {
            auto   af      = address.address4.sin_family;
            size_t len     = af == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
            char   *buffer = new char[len]{ 0 };
            int    res;

            if( af == AF_INET ) {
                res = uv_inet_ntop( af, &address.address4.sin_addr, buffer, len );

            } else {
                res = uv_inet_ntop( af, &address.address6.sin6_addr, buffer, len );
            }

            if( res != 0 ) {
                delete[] buffer;

                throw ::uv::Exception( res );
            }

            std::string result( buffer );

            delete[] buffer;

            return result;
        }

        void pton( const std::string &address, address_t *target ) {
            assert( target != nullptr );

            auto af = address.find( ':' ) == std::string::npos ? AF_INET : AF_INET6;
            int  res;

            if( af == AF_INET ) {
                res = uv_inet_pton( af, address.c_str(), &target->address4.sin_addr );

            } else {
                res = uv_inet_pton( af, address.c_str(), &target->address6.sin6_addr );
            }

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            target->address4.sin_family = af;
        }

        inline address_t pton( const std::string &address ) {
            address_t tmp;

            pton( address, &tmp );

            return tmp;
        }

        std::vector<interface_t> interfaces() {
            uv_interface_address_t *addresses;
            int                    count;

            int res = uv_interface_addresses( &addresses, &count );

            if( res != 0 ) {
                throw ::uv::Exception( res );
            }

            std::vector<interface_t> a( &addresses[0], &addresses[count] );

            uv_free_interface_addresses( addresses, count );

            return a;
        }
    }
}

#ifdef UV_OVERLOAD_OSTREAM

#include <iomanip>

template <typename _Char>
std::basic_ostream<_Char> &
operator<<( std::basic_ostream<_Char> &out, const std::vector<uv::net::interface_t> &interfaces ) {
    for( auto &x : uv::net::interfaces()) {
        out << std::left << std::setw( INET6_ADDRSTRLEN ) << x.ip_address() << " - " << x.name << std::endl;
    }

    return out;
}

#endif

#endif //UV_NET_HPP
