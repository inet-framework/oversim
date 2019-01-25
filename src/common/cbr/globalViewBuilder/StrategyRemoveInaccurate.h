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
 * @StrategyRemoveInaccurate.h
 * @author Daniel Lienert
 */

#ifndef STRATEGYREMOVEINACCURATE_H_
#define STRATEGYREMOVEINACCURATE_H_

#include <StrategyRemoveCoords.h>
#include <algorithm>

class StrategyRemoveInaccurate : public StrategyRemoveCoords {
public:
	StrategyRemoveInaccurate();
	virtual ~StrategyRemoveInaccurate();

	/**
	 * need own implementation here, cause we need the error verctor as well
	 */
	virtual void setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo);

protected:

	/**
	 * sorts the coordinatesVector by the last Value an removes the amount of entrys specified
	 * @param coordinatesVector* coords coordVector
	 * int entrysToRemove
	 */
	void removeCoordinates(coordinatesVector* coords, int entrysToRemove);

private:


};

#endif /* STRATEGYREMOVEINACCURATE_H_ */
