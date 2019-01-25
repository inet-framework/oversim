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
 * @StrategyTreeTest.h
 * @author Daniel Lienert
 */

#ifndef STRATEGYTREETEST_H_
#define STRATEGYTREETEST_H_

#include <map>
#include <oversim_mapset.h>

#include <TransportAddress.h>
#include <AbstractSendStrategy.h>

#include <NodeHandle.h>
#include <GlobalNodeList.h>
#include <OverlayKey.h>
#include <TreeManagementMessage_m.h>

class TransportAddress;

class StrategyTreeTest : public AbstractSendStrategy {

private:

	typedef std::pair<TransportAddress, int> branchCount;
	typedef UNORDERED_MAP<TransportAddress, int, TransportAddress::hashFcn> nodeCountMap;

	nodeCountMap branchCountMap;

public:
	StrategyTreeTest();
	virtual ~StrategyTreeTest();

	virtual void handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall);
	virtual void setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo);
	virtual GlobalViewBuilderCall* getCoordinateMessage();

	virtual std::vector<std::vector<double> > getGlobalViewData();
	virtual std::string getStrategyDataStatus();

	virtual void cleanUpCoordData(const treeNodeMap& currentTreeChildNodes);

	virtual std::string getStrategyCombinedParams();

protected:
	void setBranchCount(const NodeHandle& treeNode, int count);
	int getNodeCount();
};

#endif /* STRATEGYTREETEST_H_ */
