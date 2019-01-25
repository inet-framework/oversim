//
// Copyright (C) 2010 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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
 * @SimpleUnderlayNCS.h
 * @author Daniel Lienert
 */

#include "SimpleUnderlayNCS.h"

#include <GlobalNodeListAccess.h>
#include <OverlayAccess.h>
#include <SimpleNodeEntry.h>
#include <SimpleInfo.h>




void SimpleUnderlayNCS::init(NeighborCache* neighorCache) {
    this->neighborCache = neighorCache;
    SimpleUnderlayCoordsInfo::setDimension(2); //todo set value by parameter

    PeerInfo* peerInfo =
       GlobalNodeListAccess().get()->getPeerInfo(this->neighborCache->overlay->
                                                 getThisNode().getIp());

    if(peerInfo == NULL) {
        throw cRuntimeError("No PeerInfo Found");
    }

    SimpleNodeEntry* entry = dynamic_cast<SimpleInfo*>(peerInfo)->getEntry();

    ownCoords = new SimpleUnderlayCoordsInfo();
    for (uint8_t i = 0; i < entry->getDim(); i++) {
        ownCoords->setCoords(i, entry->getCoords(i));
    }
}

AbstractNcsNodeInfo* SimpleUnderlayNCS::createNcsInfo(const std::vector<double>& fakeCoords) const {

    //std::stringstream tempStr;

    PeerInfo* peerInfo =
       GlobalNodeListAccess().get()->getPeerInfo(this->neighborCache->overlay->
                                                 getThisNode().getIp());

    if(peerInfo == NULL) {
        throw cRuntimeError("No PeerInfo Found");
    }

    SimpleNodeEntry* entry = dynamic_cast<SimpleInfo*>(peerInfo)->getEntry();

    SimpleUnderlayCoordsInfo* info = new SimpleUnderlayCoordsInfo();
    for (uint8_t i = 0; i < entry->getDim(); i++) {
        info->setCoords(i, entry->getCoords(i));
    }

    return info;
}

AbstractNcsNodeInfo* SimpleUnderlayNCS::getUnvalidNcsInfo() const
{
    return new SimpleUnderlayCoordsInfo();
}

const AbstractNcsNodeInfo& SimpleUnderlayNCS::getOwnNcsInfo() const {

    //return *createNcsInfo(*new std::vector<double>);
    return *ownCoords;
}

Prox SimpleUnderlayNCS::getCoordinateBasedProx(const AbstractNcsNodeInfo& abstractInfo) const {
    //createNcsInfo(*new std::vector<double>);
    return ownCoords->getDistance(abstractInfo);
}
