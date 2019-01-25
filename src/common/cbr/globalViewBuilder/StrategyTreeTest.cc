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
 * @StrategyTreeTest.cc
 * @author Daniel Lienert
 */


#include "StrategyTreeTest.h"

StrategyTreeTest::StrategyTreeTest() {
	// TODO Auto-generated constructor stub
}

StrategyTreeTest::~StrategyTreeTest() {
	// TODO Auto-generated destructor stub
}

void StrategyTreeTest::handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall) {

	TreeTestStrategyCall* treeTestStrategyCall = dynamic_cast<TreeTestStrategyCall*>(globalViewBuilderCall);

	setBranchCount(treeTestStrategyCall->getSrcNode(),treeTestStrategyCall->getNodeCount());

}

GlobalViewBuilderCall* StrategyTreeTest::getCoordinateMessage() {

	TreeTestStrategyCall* msg = new TreeTestStrategyCall("TreeTestStrategyCall");

	msg->setNodeCount(getNodeCount());
	msg->setBitLength(TREETESTSTRATEGYCALL_L(msg));

	return msg;
}

std::string StrategyTreeTest::getStrategyDataStatus() {

	std::stringstream tempStr;

	tempStr << "BNC: " << getNodeCount() << " | BC: " << branchCountMap.size();

	return tempStr.str();
}

std::string StrategyTreeTest::getStrategyCombinedParams() {

	std::stringstream tempStr;
	tempStr << "treeTest";

	return tempStr.str();
}

void StrategyTreeTest::setBranchCount(const NodeHandle& treeNode, int count) {

	if(branchCountMap.find(treeNode) != branchCountMap.end()) {
		branchCountMap.find(treeNode)->second = count;
	} else {
		branchCountMap.insert(branchCount(treeNode, count));
	}

}

void StrategyTreeTest::cleanUpCoordData(const treeNodeMap& currentTreeChildNodes) {

	nodeCountMap::const_iterator branchIterator = branchCountMap.begin();

	while(branchIterator != branchCountMap.end()) {

		if(currentTreeChildNodes.find(branchIterator->first) == currentTreeChildNodes.end() && branchIterator->first != thisNode) {
			branchCountMap.erase(branchIterator);
		}

		branchIterator++;
	}

}

int StrategyTreeTest::getNodeCount() {

	int nodeCount = 0;

	nodeCountMap::const_iterator branchCountMapIterator = branchCountMap.begin();

	while(branchCountMapIterator != branchCountMap.end()) {
		nodeCount += branchCountMapIterator->second;
		++branchCountMapIterator;
	}

	return nodeCount;
}

void StrategyTreeTest::setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo) {
	setBranchCount(thisNode, 1);
}

std::vector<std::vector<double> > StrategyTreeTest::getGlobalViewData() {
	std::vector<std::vector<double> > test;
	return test;
}
