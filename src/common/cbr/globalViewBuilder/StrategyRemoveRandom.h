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
 * @strategyRemoveRandom.h
 * @author Daniel Lienert
 */

#ifndef STRATEGYREMOVERANDOM_H_
#define STRATEGYREMOVERANDOM_H_

#include <time.h>
#include <StrategyRemoveCoords.h>

class NeighborCache;

class StrategyRemoveRandom : public StrategyRemoveCoords {
public:
	StrategyRemoveRandom();
	virtual ~StrategyRemoveRandom();

protected:

	void removeCoordinates(coordinatesVector* coords, int entrysToRemove);
};

#endif /* STRATEGYREMOVERANDOM_H_ */
