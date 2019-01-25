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
 * @StrategyRegions.h
 * @author Daniel Lienert
 */

#ifndef STRATEGYREGIONS_H_
#define STRATEGYREGIONS_H_

#include <oversim_mapset.h>
#include <RegionDataContainer.h>
#include <AbstractSendStrategy.h>

class GlobalViewBuilderMessage;
class RegionDataContainer;

typedef std::pair<long, int> regionCountPair;
typedef UNORDERED_MAP<long, int> regionCountMap;

class StrategyRegions : public AbstractSendStrategy{
public:
	StrategyRegions();
	virtual ~StrategyRegions();

	virtual GlobalViewBuilderCall* getCoordinateMessage();
	virtual void handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall);
	virtual std::string getStrategyDataStatus();

	virtual std::vector<std::vector<double> > getGlobalViewData();

	virtual void setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo);

	virtual void cleanUpCoordData(const treeNodeMap& currentTreeChildNodes);

	virtual std::string getStrategyCombinedParams();

	void debugRegionData();

protected:

	/**
	 * The regionVector is a vector of integer values, each value represents a coordinate vector
	 */
	typedef std::vector<long> regionVector;

	typedef std::pair<TransportAddress, regionCountMap > nodeRegionData;
	typedef UNORDERED_MAP<TransportAddress, regionCountMap, TransportAddress::hashFcn> branchRegionDataMap;

	branchRegionDataMap regionData;

	/**
	 * convert a vector of doubles (=dimensions) to a single integer
	 * @param std::vector<double> coordVector
	 * @return long region ID
	 */
	long convertCoordVectorToRegion(std::vector<double> coordVector);

	/**
	 * convert the region ID to a vector of doubles
	 * @param int region
	 * @param int dimCount count of dimension of the original coordVector
	 * @return std::vector<double> coordVector
	 */
	std::vector<double> convertRegionToCoordVector(int region, int dimCount);

	const regionCountMap getCombinedRegionsMap();

	/**
	 * set the regions delivered by a child to our regionCountMap
	 * @param node NodeHandle assign the map to this node
	 * @param countMap regionCountMap the map of the node
	 */
	void setBranchCoordinates(const NodeHandle& node, regionCountMap countMap);

	int getSizePerDim();

	int getMaxSpread();

	double checkMaxSpread(double dimValue);

private:

};

#endif /* STRATEGYREGIONS_H_ */
