//
// Copyright (C) 2010 Karlsruhe Institute of Technology (KIT),
//                    Institute of Telematics
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
 * @file PeerStorage.h
 * @author Ingmar Baumgart
 */

#ifndef __PEERSTORAGE_H__
#define __PEERSTORAGE_H__

#include <vector>
#include <list>

#include <oversim_mapset.h>
#include <IPvXAddress.h>
#include <TransportAddress.h>
#include <HashFunc.h>
#include <PeerInfo.h>

/**
 * AddrPerOverlay contains the
 * TransportAddress and bootstrap status
 * for every overlay this node is part of
 */
struct AddrPerOverlay
{
    int32_t overlayId;
    TransportAddress* ta;
    bool bootstrapped; /**< true if node has bootstrapped in this overlay */
    uint32_t overlayPeerVectorIndex;
};

class AddrPerOverlayVector : public std::vector<AddrPerOverlay>
{
public:
    ~AddrPerOverlayVector() {
        for (iterator it = begin(); it != end(); it++) {
            delete it->ta;
        }
    }

    const AddrPerOverlayVector::iterator getIterForOverlayId(int32_t overlayId) {
        iterator it;
        for (it = begin(); it != end(); it++) {
            if (it->overlayId == overlayId) return it;
        }

        return it;
    };

    TransportAddress* getAddrForOverlayId(int32_t overlayId) {
        iterator it = getIterForOverlayId(overlayId);

        if (it != end()) {
            return it->ta;
        }

        return NULL;
    };

    void setAddrForOverlayId(TransportAddress* addr, int32_t overlayId) {
        for (iterator it = begin(); it != end(); it++) {
            if (it->overlayId == overlayId) {
                delete it->ta;
                it->ta = addr;
                return;
            }
        }

        AddrPerOverlay apo;
        apo.overlayId = overlayId;
        apo.bootstrapped = false;
        apo.ta = addr;
        push_back(apo);
    };
};

/**
 * BootstrapEntry consists of
 * TransportAddress and PeerInfo
 * and is used (together with
 * IPvXAddress) as an entry in the peerSet
 */
struct BootstrapEntry
{
    AddrPerOverlayVector addrVector;
    PeerInfo* info;
    uint32_t peerVectorIndex;
    friend std::ostream& operator<<(std::ostream& Stream, const BootstrapEntry& entry);
};

typedef UNORDERED_MAP<IPvXAddress, BootstrapEntry> PeerHashMap;


/**
 *
 * @author IngmarBaumgart
 */
class PeerStorage
{
public:
    ~PeerStorage();

    size_t size();
    const PeerHashMap::iterator find(const IPvXAddress& ip);
    const PeerHashMap::iterator begin();
    const PeerHashMap::iterator end();

    std::pair<const PeerHashMap::iterator, bool> insert(const std::pair<IPvXAddress, BootstrapEntry>& element);
    void erase(const PeerHashMap::iterator it);

    void registerOverlay(const PeerHashMap::iterator it,
                         const NodeHandle& peer,
                         int32_t overlayId);

    const PeerHashMap::iterator getRandomNode(int32_t overlayId,
                                              int32_t nodeType,
                                              bool bootstrappedNeeded,
                                              bool inoffensiveNeeded);

    void setMalicious(const PeerHashMap::iterator it, bool malicious);

    void setBootstrapped(const PeerHashMap::iterator it, int32_t overlayId,
                         bool bootstrapped);

    const PeerHashMap& getPeerHashMap() { return peerHashMap; };

private:
    typedef std::vector<std::vector<PeerHashMap::iterator> > PeerVector;

    void insertMapIteratorIntoVector(PeerVector& peerVector,
                                     PeerHashMap::iterator it);

    void removeMapIteratorFromVector(PeerVector& peerVector,
                                     PeerHashMap::iterator it);

    inline size_t offsetSize();
    inline uint8_t calcOffset(bool bootstrapped, bool malicious);

    PeerHashMap peerHashMap; /* hashmap contain all nodes */
    PeerVector globalPeerVector; /* vector with iterators to peerHashMap */
    std::map<int32_t, PeerVector> overlayPeerVectorMap; /* vector of vectors (for each overlayId) with iterators to peerHashMap */
    std::vector<std::vector<uint32_t> >freeVector;
};

#endif
