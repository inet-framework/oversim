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
 * @AbstractSendStrategy.cc
 * @author Daniel Lienert
 */

#include "AbstractSendStrategy.h"
#include <GlobalViewBuilder.h>

AbstractSendStrategy::AbstractSendStrategy() {
	// TODO Auto-generated constructor stub
}

AbstractSendStrategy::~AbstractSendStrategy() {
	// TODO Auto-generated destructor stub
}

void AbstractSendStrategy::initialize(GlobalViewBuilder* globalViewBuilder) {
	this->globalViewBuilder = globalViewBuilder;

	initializeStrategy();
}

void AbstractSendStrategy::initializeStrategy() {
}

void AbstractSendStrategy::handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall) {
}

GlobalViewBuilderCall* AbstractSendStrategy::getCoordinateMessage() {
	return new GlobalViewBuilderCall;
}

void AbstractSendStrategy::setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo) {
}

std::vector<std::vector<double> > getGlobalViewData() {
	std::vector<std::vector<double> > tmpVector;
	return tmpVector;
}

std::string AbstractSendStrategy::getStrategyDataStatus(){
	return "-";
}

void AbstractSendStrategy::cleanUpCoordData(const treeNodeMap& currentTreeChildNodes) {
}

void AbstractSendStrategy::setThisNode(const NodeHandle thisNode) {
	this->thisNode = thisNode;
}
