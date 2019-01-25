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
 * @strategySimplifyCoords.h
 * @author Daniel Lienert
 */

#ifndef STRATEGYSIMPLIFYCOORDS_H_
#define STRATEGYSIMPLIFYCOORDS_H_

#include <oversim_mapset.h>
#include <SimpleCoordDataContainer.h>
#include <AbstractSendStrategy.h>

class GlobalViewBuilderMessage;
class SimpleCoordinate;

typedef std::pair<SimpleCoordinate, int> simpleCoordPair;
typedef UNORDERED_MAP<SimpleCoordinate, int, SimpleCoordinate::hashFcn> simpleCoordCountMap;

class StrategySimplifyCoords : public AbstractSendStrategy {
public:
	StrategySimplifyCoords();
	virtual ~StrategySimplifyCoords();

	virtual GlobalViewBuilderCall* getCoordinateMessage();
	virtual void handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall);
	virtual std::string getStrategyDataStatus();

	virtual std::vector<std::vector<double> > getGlobalViewData();

	virtual void setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo);

	virtual void cleanUpCoordData(const treeNodeMap& currentTreeChildNodes);

	virtual std::string getStrategyCombinedParams();

protected:

	typedef std::pair<TransportAddress, simpleCoordCountMap > nodeCoordData;
	typedef UNORDERED_MAP<TransportAddress, simpleCoordCountMap, TransportAddress::hashFcn> branchCoordDataMap;

	branchCoordDataMap coordData;

	/**
	 * set the coordCountMap delivered by a child to our coordCountMap
	 * @param node NodeHandle assign the map to this node
	 * @param countMap simpleCoordCountMap the map of the node
	 */
	void setBranchCoordinates(const NodeHandle& node, simpleCoordCountMap countMap);

	/**
	 * simplify every dimension of the given coordVector by the simplifyFactor
	 * @param std::vector<double> coordvector original double vector
	 * @return std::vector<int> simplified vector
	 */
	SimpleCoordinate simplify(std::vector<double> coordVector);


	/**
	 * decode the simplified vector to the original interval
	 * @param std::vector<int> simplifiedVector
	 * @return std::vector<double> vector in original intervall
	 */
	std::vector<double>decode(SimpleCoordinate simplifiedVector);


	const simpleCoordCountMap getCombinedCoordCountMap();
};

#endif /* STRATEGYSIMPLIFYCOORDS_H_ */
