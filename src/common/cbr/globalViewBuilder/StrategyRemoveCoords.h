/*
 * StrategyRemoveCoords.h
 *
 *  Created on: 17.05.2010
 *      Author: user
 */

#ifndef STRATEGYREMOVECOORDS_H_
#define STRATEGYREMOVECOORDS_H_

#include <StrategySendAll.h>

class NeighborCache;

class StrategyRemoveCoords : public StrategySendAll {
public:
	StrategyRemoveCoords();
	virtual ~StrategyRemoveCoords();

	virtual GlobalViewBuilderCall* getCoordinateMessage();

	virtual std::string getStrategyCombinedParams();

protected:

void processCoordinates(coordinatesVector* coords);

void removeCoordinatesByPercentage(coordinatesVector* coords, int percentage);

void removeCoordinatesByCoordLimit(coordinatesVector* coords, int coordLimit);

void removeCoordinatesByTrafficLimit(coordinatesVector* coords, int trafficLimit);

virtual void removeCoordinates(coordinatesVector* coords, int entrysToRemove);

};

#endif /* STRATEGYREMOVECOORDS_H_ */
