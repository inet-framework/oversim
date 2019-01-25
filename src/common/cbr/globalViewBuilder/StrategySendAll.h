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
 * @strategySendAll.h
 * @author Daniel Lienert
 */

#ifndef STRATEGYSENDALL_H_
#define STRATEGYSENDALL_H_

#include <oversim_mapset.h>

#include <CoordDataContainer.h>
#include <AbstractSendStrategy.h>

class GlobalViewBuilderMessage;
class CoordDataContainer;
//class SendAllStrategyMessage;

class StrategySendAll : public AbstractSendStrategy {
public:
	StrategySendAll();
	virtual ~StrategySendAll();

	virtual void setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo);

	virtual GlobalViewBuilderCall* getCoordinateMessage();
	virtual void handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall);
	virtual std::string getStrategyDataStatus();

	virtual std::vector<std::vector<double> > getGlobalViewData();

	virtual void cleanUpCoordData(const treeNodeMap& currentTreeChildNodes);

	virtual std::string getStrategyCombinedParams();

protected:
	/**
	 * The coordinatesVector is a vector of coordinates, where a coordinate itself is a vector of doubles (=Dimensions)
	 */
	typedef std::vector<std::vector<double> > coordinatesVector;
	typedef std::pair<TransportAddress, coordinatesVector > nodeCoordData;
	typedef UNORDERED_MAP<TransportAddress, coordinatesVector, TransportAddress::hashFcn> coordDataMap;


	coordDataMap coordData;

	/**
	 * Set the CoordinatesVector to the map, identified by the sender node
	 * @param node NodeHandle to identify the sending node
	 * @param coordsVector coordinatesVector with the coordinates of all nodes in this branch
	 */
	void setBranchCoordinates(const NodeHandle& node, coordinatesVector coordsVector);

	/**
	 * Combine the coordinates of coordData map Structure to a single Vector of coordinates
	 * @return single vector of coordinate vectors
	 */
	const std::vector<std::vector<double> > getCombinedCoordsVector();

	/**
	 * calculate and return the size of the used coordinate data structure
	 * return int size
	 */
	virtual int getSizeOfCoordVector(const coordinatesVector& combinedCoordsVector);

	virtual int getSizeOfSingleCoordinate(const coordinatesVector& combinedCoordsVector);
};

#endif /* STRATEGYSENDALL_H_ */
