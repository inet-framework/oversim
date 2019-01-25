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
 * @SimpleCoordDataContainer.h
 * @author Daniel Lienert
 */

#ifndef SIMPLECOORDINATE_H_
#define SIMPLECOORDINATE_H_

#include <oversim_mapset.h>
#include <math.h>
#include <cstddef>
#include <vector>

class SimpleCoordinate {

public:
	/**
	 * defines a hash function for SimpleCoordinate
	 */
	class hashFcn
	{
	public:
		size_t operator()( const SimpleCoordinate& h1 ) const
		{
			return h1.hash();
		}
	};

	SimpleCoordinate();
	virtual ~SimpleCoordinate();

	/**
	 * comapares this to a given coordinate
	 * @param compSimpleCoordinate SimpleCoordinate to compare to
	 */
	bool operator< ( const SimpleCoordinate& compSimpleCoordinate ) const;

	/**
	 * comapares this to a given coordinate
	 * @param compSimpleCoordinate SimpleCoordinate to compare to
	 */
	bool operator> ( const SimpleCoordinate& compSimpleCoordinate ) const;


	/**
	 * comapares this to a given coordinate
	 * @param compSimpleCoordinate SimpleCoordinate to compare to
	 */
	bool operator== ( const SimpleCoordinate& compSimpleCoordinate ) const;

	/**
	 * returns a hash value
	 */
	size_t hash() const;

	std::vector<int> coordData;

protected:

	/**
	 * Unifies all compare operations in one method
	 *
	 * @param compSimpleCoordinate simpleCoord to compare with
	 * @return int -1 if smaller, 0 if equal, 1 if greater
	 */
	int compareTo(const SimpleCoordinate& compSimpleCoordinate) const;

};

#endif /* SIMPLECOORDINATE_H_ */



#ifndef SIMPLECOORDDATACONTAINER_H_
#define SIMPLECOORDDATACONTAINER_H_

class SimpleCoordDataContainer {

public:
	SimpleCoordDataContainer();
	virtual ~SimpleCoordDataContainer();

	typedef UNORDERED_MAP<SimpleCoordinate, int, SimpleCoordinate::hashFcn> simpleCoordCountMap;

	simpleCoordCountMap coordData;

	/**
	 * calculate the size of the coord data map
	 * @return int size in bit
	 */
	int getBitLength();

};

#endif /* SIMPLECOORDDATACONTAINER_H_ */
