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
 * @StrategySimplifyCoords.cc
 * @author Daniel Lienert
 */

#include "StrategySimplifyCoords.h"
#include <GlobalViewBuilder.h>

StrategySimplifyCoords::StrategySimplifyCoords() {
	// TODO Auto-generated constructor stub
}

StrategySimplifyCoords::~StrategySimplifyCoords() {
	// TODO Auto-generated destructor stub
}

void StrategySimplifyCoords::setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo) {

	std::vector<double> tmpDimVector = ncsInfo.getCoords();
	simpleCoordCountMap tmpCountMap;
	tmpCountMap.insert(simpleCoordPair(simplify(tmpDimVector),1));

	setBranchCoordinates(thisNode, tmpCountMap);	// i am my own branch here ...
}

void StrategySimplifyCoords::setBranchCoordinates(const NodeHandle& node, simpleCoordCountMap countMap) {

	if(coordData.find(node) != coordData.end()) {
		coordData.find(node)->second = countMap;
	} else {
		coordData.insert(nodeCoordData(node, countMap));
	}
}

GlobalViewBuilderCall* StrategySimplifyCoords::getCoordinateMessage() {

	SimpleCoordStrategyCall* msg = new SimpleCoordStrategyCall("SimpleCoordStrategyCall");
	SimpleCoordDataContainer tmpSimpleCoordContainer;

	tmpSimpleCoordContainer.coordData = getCombinedCoordCountMap();

	msg->setCoordData(tmpSimpleCoordContainer);
	msg->setBitLength(SIMPLECOORDSTRATEGYCALL_L(msg));

	return msg;
}

const simpleCoordCountMap StrategySimplifyCoords::getCombinedCoordCountMap() {
	simpleCoordCountMap combinedCoordMap;
	simpleCoordCountMap branchCoordMap;

	branchCoordDataMap::const_iterator coordDataIterator = coordData.begin();

	while(coordDataIterator != coordData.end()) {

		branchCoordMap = coordDataIterator->second;

		simpleCoordCountMap::iterator bSCMapIterator = branchCoordMap.begin();

		while(bSCMapIterator != branchCoordMap.end()) {

			if(combinedCoordMap.find(bSCMapIterator->first) == combinedCoordMap.end()) {
				combinedCoordMap.insert(simpleCoordPair(bSCMapIterator->first, bSCMapIterator->second));
			} else {
				combinedCoordMap.find(bSCMapIterator->first)->second += bSCMapIterator->second;
			}

			bSCMapIterator++;
		}

		coordDataIterator++;
	}

	return combinedCoordMap;
}

void StrategySimplifyCoords::handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall) {
	SimpleCoordStrategyCall* simpleCoordStrategyCall = dynamic_cast<SimpleCoordStrategyCall*>(globalViewBuilderCall);

	setBranchCoordinates(simpleCoordStrategyCall->getSrcNode(), simpleCoordStrategyCall->getCoordData().coordData);
}

std::string StrategySimplifyCoords::getStrategyDataStatus() {
	std::stringstream tempStr;

	simpleCoordCountMap coordMap = getCombinedCoordCountMap();
	simpleCoordCountMap::const_iterator coordIterator = coordMap.begin();

	while(coordIterator != coordMap.end()) {

		tempStr << "(";

		std::vector<int> vDim = coordIterator->first.coordData;
		std::vector<int>::const_iterator vDimIterator = vDim.begin();
		while(vDimIterator != vDim.end()) {

			tempStr << (*vDimIterator) << ";";

			vDimIterator++;
		}

		tempStr << "):" << coordIterator->second << "|";

		coordIterator++;
	}



	return tempStr.str();
}


std::string StrategySimplifyCoords::getStrategyCombinedParams() {

	std::stringstream tempStr;
	tempStr << globalViewBuilder->parProxy("gvbSendStrategy").stdstringValue();
	tempStr << "/";
	tempStr << globalViewBuilder->parProxy("gvbStrategySimplifyCoordsFactor").longValue();

	return tempStr.str();
}


void StrategySimplifyCoords::cleanUpCoordData(const treeNodeMap& currentTreeChildNodes) {

	branchCoordDataMap::const_iterator coordMapIterator = coordData.begin();

	while(coordMapIterator != coordData.end()) {

		if(currentTreeChildNodes.find(coordMapIterator->first) == currentTreeChildNodes.end() && coordMapIterator->first != thisNode) {
			coordData.erase(coordMapIterator);
		}

		coordMapIterator++;
	}
}



SimpleCoordinate StrategySimplifyCoords::simplify(std::vector<double> coordVector) {

	int simplifyFactor = globalViewBuilder->parProxy("gvbStrategySimplifyCoordsFactor");
	int simplified = 0;
	SimpleCoordinate outVector;
	std::vector<double>::const_iterator dimIterator = coordVector.begin();

	while(dimIterator != coordVector.end()) {

		simplified = static_cast<int>((*dimIterator)*simplifyFactor + 0.5);
		outVector.coordData.push_back(simplified);
		dimIterator++;
	}

	return outVector;
}

std::vector<double> StrategySimplifyCoords::decode(SimpleCoordinate simplifiedVector) {

	int simplifyFactor = globalViewBuilder->parProxy("gvbStrategySimplifyCoordsFactor");
	double dimension = 0.0;

	std::vector<int>::const_iterator dimIterator = simplifiedVector.coordData.begin();
	std::vector<double> outVector;

	while(dimIterator != simplifiedVector.coordData.end()) {
		dimension = static_cast<double>(*dimIterator) / static_cast<double>(simplifyFactor);
		outVector.push_back(dimension);

		dimIterator++;
	}

	return outVector;

}

std::vector<std::vector<double> > StrategySimplifyCoords::getGlobalViewData() {

	simpleCoordCountMap coordMap = getCombinedCoordCountMap();
	simpleCoordCountMap::const_iterator coordIterator = coordMap.begin();

	std::vector<std::vector<double> > globalViewData;
	int i;

	while(coordIterator != coordMap.end()) {

		for(i=1; i<=coordIterator->second; i++) {
			globalViewData.push_back(decode(coordIterator->first));
		}

		coordIterator++;
	}

	return globalViewData;

}
