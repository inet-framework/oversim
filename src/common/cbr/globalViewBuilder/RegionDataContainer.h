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
 * @RegionDataContainer.h
 * @author Daniel Lienert
 */

#ifndef REGIONDATACONTAINER_H_
#define REGIONDATACONTAINER_H_

#include <ostream>

#include <oversim_mapset.h>


typedef UNORDERED_MAP<long, int> regionCountMap;

std::ostream& operator<<(std::ostream& os, const regionCountMap& info);

class RegionDataContainer
{
public:
    RegionDataContainer();
    virtual ~RegionDataContainer();



    regionCountMap regionData;

    /**
     * calculate the size of the region data map
     * @return int size in bit
     */
    int getBitLength();
};

#endif /* REGIONDATACONTAINER_H_ */
