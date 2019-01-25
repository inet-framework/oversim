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
 * @SimpleCoordDataContainer.cc
 * @author Daniel Lienert
 */

#include "SimpleCoordDataContainer.h"

SimpleCoordDataContainer::SimpleCoordDataContainer() {
	// TODO Auto-generated constructor stub

}

SimpleCoordDataContainer::~SimpleCoordDataContainer() {
	// TODO Auto-generated destructor stub
}

int SimpleCoordDataContainer::getBitLength() {
	int size = 0;

	simpleCoordCountMap::const_iterator coordIterator = coordData.begin();
	size += sizeof(coordData);

	while(coordIterator != coordData.end()) {
		size += sizeof(coordIterator->first);
		size += sizeof(coordIterator->second);

		coordIterator++;
	}

	return (size*8);
}



SimpleCoordinate::SimpleCoordinate() {
	// TODO Auto-generated constructor stub

}

SimpleCoordinate::~SimpleCoordinate() {
	// TODO Auto-generated destructor stub
}

int SimpleCoordinate::compareTo(const SimpleCoordinate& compSimpleCoordinate) const {

	std::vector<int> compCoord = compSimpleCoordinate.coordData;

	if(coordData.size() != compCoord.size()) {
		throw std::exception();
	}

	for(unsigned int i = 0; i < coordData.size(); i++) {
		if(coordData[i] < compCoord[i]) return -1;
		if(coordData[i] > compCoord[i]) return 1;
	}

	return 0;
}

size_t SimpleCoordinate::hash() const {

	long hashValue = 0;

	for(unsigned int i = 0; i < coordData.size(); i++) {
		hashValue += pow(coordData[i],i);
	}

	return (size_t)hashValue;
}

bool SimpleCoordinate::operator< ( const SimpleCoordinate& compSimpleCoordinate ) const {
	return compareTo(compSimpleCoordinate) == -1;
}

bool SimpleCoordinate::operator> ( const SimpleCoordinate& compSimpleCoordinate ) const {
	return compareTo(compSimpleCoordinate) == 1;
}

bool SimpleCoordinate::operator== ( const SimpleCoordinate& compSimpleCoordinate ) const {
	return compareTo(compSimpleCoordinate) == 0;
}
