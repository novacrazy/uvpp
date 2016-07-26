//
// Created by Aaron on 7/26/2016.
//

#ifndef UV_NET_HPP
#define UV_NET_HPP

#include "defines.hpp"
#include "exception.hpp"
#include "fwd.hpp"

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

#endif //UV_NET_HPP
