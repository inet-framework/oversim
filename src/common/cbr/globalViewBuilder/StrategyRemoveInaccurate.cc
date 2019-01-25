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
 * @StrategyRemoveInaccurate.c
 * @author Daniel Lienert
 */
#include "StrategyRemoveInaccurate.h"

bool compareCoordsByErrorVector(const std::vector<double>& va, const std::vector<double>& vb) {
	return (va.at(va.size()-1) < vb.at(vb.size()-1));
}

StrategyRemoveInaccurate::StrategyRemoveInaccurate() {
	// TODO Auto-generated constructor stub

}

StrategyRemoveInaccurate::~StrategyRemoveInaccurate() {
	// TODO Auto-generated destructor stub
}

void StrategyRemoveInaccurate::removeCoordinates(coordinatesVector* coords, int entrysToRemove) {

	std::sort(coords->begin(), coords->end(), compareCoordsByErrorVector);	//sorts in ascending order

	for(int i = 0; i < entrysToRemove; i++) {
		coords->pop_back();
	}
}

void StrategyRemoveInaccurate::setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo) {

	std::vector<double> tmpDimVector = ncsInfo;
	coordinatesVector tmpCoordsVector;
	tmpCoordsVector.push_back(tmpDimVector);

	setBranchCoordinates(thisNode, tmpCoordsVector);	// i am my own branch here ...
}

