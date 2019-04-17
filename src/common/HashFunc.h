//
// Copyright (C) 2006 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
 * @file
 * @author Stephan Krause
 */

#ifndef __HASHFUNC
#define __HASHFUNC

#include "common/oversim_mapset.h"
#include "common/oversim_byteswap.h"

#include "inet/networklayer/common/L3Address.h"
#include "common/TransportAddress.h"

#if defined(HAVE_GCC_TR1) || defined(HAVE_MSVC_TR1)
namespace std { namespace tr1 {
#else
namespace __gnu_cxx {
#endif

/**
 * defines a hash function for L3Address
 */
template<> struct hash<L3Address> : std::unary_function<L3Address, std::size_t>
{
    /**
     * hash function for L3Address
     *
     * @param addr the L3Address to hash
     * @return the hashed L3Address
     */
    std::size_t operator()(const L3Address& addr) const
    {
        L3Address::AddressType addrType = addr.getType();
        switch (addrType) {
            case L3Address::AddressType::NONE:
                return 0;
            case L3Address::AddressType::IPv4:
                return bswap_32(addr.toIPv4().getInt());
            case L3Address::AddressType::IPv6: {
                IPv6Address ipv6Addr = addr.toIPv6();
                return ((bswap_32(ipv6Addr.words()[0])) ^
                       (bswap_32(ipv6Addr.words()[1])) ^
                       (bswap_32(ipv6Addr.words()[2])) ^
                       (bswap_32(ipv6Addr.words()[3])));
            }
            case L3Address::AddressType::MAC: {
                 uint64_t macaddr = addr.toMAC().getInt();
                 return ((bswap_32((uint32_t)(macaddr >> 32))) ^
                         (bswap_32((uint32_t)(macaddr))));
            }
            case L3Address::AddressType::MODULEPATH:
                return bswap_32(addr.toModulePath().getId());
            case L3Address::AddressType::MODULEID:
                return bswap_32(addr.toModuleId().getId());
            default:
                throw cRuntimeError("unaccepted address type");
                break;
        }
    }
};

/**
 * defines a hash function for TransportAddress
 */
template<> struct hash<TransportAddress> : std::unary_function<TransportAddress, std::size_t>
{
    /**
     * hash function for TransportAddress
     *
     * @param addr the TransportAddress to hash
     * @return the hashed TransportAddress
     */
    std::size_t operator()(const TransportAddress& addr) const
    {
        hash<L3Address> l3hash;
        return l3hash(addr.getIp());
    }
};

}
#if defined(HAVE_GCC_TR1) || defined(HAVE_MSVC_TR1)
}
#endif

#endif
