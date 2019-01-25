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
 * @RegionDataContainer.cc
 * @author Daniel Lienert
 */

#include "RegionDataContainer.h"

RegionDataContainer::RegionDataContainer()
{
    // TODO Auto-generated constructor stub

}

RegionDataContainer::~RegionDataContainer()
{
    // TODO Auto-generated destructor stub
}

int RegionDataContainer::getBitLength()
{
    int size = 0;

    regionCountMap::const_iterator regionIterator = regionData.begin();

    size += sizeof(regionData);

    while(regionIterator != regionData.end()) {
        size += sizeof(regionIterator->first);
        size += sizeof(regionIterator->second);

        regionIterator++;
    }

    return (size * 8);
}


std::ostream& operator<<(std::ostream& os, const regionCountMap& map)
{
    for (regionCountMap::const_iterator it = map.begin(); it != map.end(); ++it) {
        os << it->first << it->second;
    }
    return os;
}

