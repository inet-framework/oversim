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

#ifndef SIMPLEUNDERLAYNCS_H_
#define SIMPLEUNDERLAYNCS_H_

#include "CoordinateSystem.h"
#include <NeighborCache.h>

class SimpleUnderlayNCS: public AbstractNcs
{
public:
    SimpleUnderlayNCS() { ownCoords = NULL; };
    virtual ~SimpleUnderlayNCS() { delete ownCoords; };

	virtual void init(NeighborCache* neighorCache);

	virtual AbstractNcsNodeInfo* getUnvalidNcsInfo() const;

	virtual bool isAdapting() { return false; }
	AbstractNcsNodeInfo* createNcsInfo(const std::vector<double>& coords) const;
	Prox getCoordinateBasedProx(const AbstractNcsNodeInfo& info) const;
	virtual const AbstractNcsNodeInfo& getOwnNcsInfo() const;

protected:

	NeighborCache* neighborCache;

	SimpleUnderlayCoordsInfo* ownCoords;

};

#endif /* SIMPLEUNDERLAYNCS_H_ */
