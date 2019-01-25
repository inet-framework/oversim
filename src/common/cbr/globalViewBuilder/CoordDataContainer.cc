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
 * @CoordDataContainer.cc
 * @author Daniel Lienert
 */

#include "CoordDataContainer.h"

CoordDataContainer::CoordDataContainer() {
	// TODO Auto-generated constructor stub

}

CoordDataContainer::~CoordDataContainer() {
	// TODO Auto-generated destructor stub
}

int CoordDataContainer::getBitLength() {
	int size = 0;

	std::vector<std::vector<double> >::const_iterator vCoordsIterator = coordinatesVector.begin();
	std::vector<double>::const_iterator dimIterator;

	while(vCoordsIterator != coordinatesVector.end()) {

		size += sizeof((*vCoordsIterator));

		dimIterator = (*vCoordsIterator).begin();
		while(dimIterator != (*vCoordsIterator).end()) {
			size += sizeof((*dimIterator));
			dimIterator++;
		}
		vCoordsIterator++;
	}

	return (size * 8);
}

