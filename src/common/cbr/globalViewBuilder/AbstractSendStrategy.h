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
 * @AbstractSendStrategy.h
 * @author Daniel Lienert
 */

#ifndef ABSTRACTSENDSTRATEGY_H_
#define ABSTRACTSENDSTRATEGY_H_

#include <vector>
#include <CoordinateSystem.h>
#include <TreeManagementMessage_m.h>
#include <TreeManagement.h>

class GlobalViewBuilder;


class AbstractSendStrategy {
public:
	AbstractSendStrategy();
	virtual ~AbstractSendStrategy();

	/**
	 * set a pointer to the neighborCache to access the optional parameters for the strategies
	 * @param neighborCache
	 */
	virtual void initialize(GlobalViewBuilder* globalViewBuilder);

	/**
	 * stub method to initialize the concrete strategy
	 */
	virtual void initializeStrategy();

	virtual void handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall);
	virtual GlobalViewBuilderCall* getCoordinateMessage();
	virtual void setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo);

	/**
	 * return the decodes global View Data
	 */
	virtual std::vector<std::vector<double> > getGlobalViewData() = 0;

	/**
	 * Set the Nodehandle of the own node to identify the own coordinates
	 */
	virtual void setThisNode(const NodeHandle thisNode);

	/**
	 * return a short status of the running send strategy
	 */
	virtual std::string getStrategyDataStatus();

	/**
	 * return a string of the currently used strategy params
	 */
	virtual std::string getStrategyCombinedParams() = 0;

	/**
	 * cleanup the coordinate map and remove data of nodes which are not longer in the branch
	 */
	virtual void cleanUpCoordData(const treeNodeMap& currentTreeChildNodes);

protected:

	NodeHandle thisNode;
	GlobalViewBuilder* globalViewBuilder;

	/**
	 * the quantity of coordinates sent by last message
	 */
	int lastSendCount;
};

#endif /* ABSTRACTSENDSTRATEGY_H_ */
