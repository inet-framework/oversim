/*
 * StrategyRemoveCoords.cc
 *
 *  Created on: 17.05.2010
 *      Author: user
 */

#include "StrategyRemoveCoords.h"
#include <GlobalViewBuilder.h>

StrategyRemoveCoords::StrategyRemoveCoords() {
	lastSendCount = 0;
}

StrategyRemoveCoords::~StrategyRemoveCoords() {
	// TODO Auto-generated destructor stub
}

GlobalViewBuilderCall* StrategyRemoveCoords::getCoordinateMessage() {

	SendAllStrategyCall* msg = new SendAllStrategyCall("sendAllStrategyCall");

	CoordDataContainer tmpCoordContainer;
	coordinatesVector sendCoordsVector = getCombinedCoordsVector();

	processCoordinates(&sendCoordsVector);

	tmpCoordContainer.coordinatesVector = sendCoordsVector;

	lastSendCount = tmpCoordContainer.coordinatesVector.size();

	msg->setCoordData(tmpCoordContainer);
	msg->setBitLength(REMOVERANDOMSTRATEGYCALL_L(msg));

	return msg;

}

void StrategyRemoveCoords::processCoordinates(coordinatesVector* coords) {
	std::string strategyMode = globalViewBuilder->parProxy("gvbStrategyRemoveCoordsMode").stdstringValue();

	if(strategyMode == "limitCoords") {
		int coordLimit = globalViewBuilder->parProxy("gvbStrategyRemoveCoordsCoordsLimit");
		removeCoordinatesByCoordLimit(coords, coordLimit);

	} else if (strategyMode == "limitTraffic") {
		int trafficLimit = globalViewBuilder->parProxy("gvbStrategyRemoveCoordsTrafficLimit");
		removeCoordinatesByTrafficLimit(coords, trafficLimit);

	} else if (strategyMode == "percentage") {
		int percentage = globalViewBuilder->parProxy("gvbStrategyRemoveCoordsPercentage");
		removeCoordinatesByPercentage(coords, percentage);

	} else if (strategyMode == "") {
		throw cRuntimeError("StrategyRemoveCoordinates::processCoordinates(): No Strategy Mode given.");

	} else {
		throw cRuntimeError("StrategyRemoveCoordinates::processCoordinates(): Unknown Strategy Mode given.");
	}
}

void StrategyRemoveCoords::removeCoordinates(coordinatesVector* coords, int entrysToRemove) {
	throw cRuntimeError("StrategyRemoveCoordinates::processCoordinates(): Choice Strategy not implemented.");
}

void StrategyRemoveCoords::removeCoordinatesByPercentage(coordinatesVector* coords, int percentage) {
	int entrysToRemove = static_cast<int>(coords->size() * (percentage / 100));
	removeCoordinates(coords, entrysToRemove);
}

void StrategyRemoveCoords::removeCoordinatesByCoordLimit(coordinatesVector* coords, int coordLimit) {
	int entrysToRemove = coords->size() - coordLimit;
	removeCoordinates(coords, entrysToRemove);
}

void StrategyRemoveCoords::removeCoordinatesByTrafficLimit(coordinatesVector* coords, int trafficLimit) {
	int coordLimit = (int)(trafficLimit / getSizeOfSingleCoordinate(*coords));
	removeCoordinatesByCoordLimit(coords, coordLimit);
}

std::string StrategyRemoveCoords::getStrategyCombinedParams() {

	std::string strategyMode = globalViewBuilder->parProxy("gvbStrategyRemoveCoordsMode").stdstringValue();

	std::stringstream tempStr;
	tempStr << globalViewBuilder->parProxy("gvbSendStrategy").stdstringValue();
	tempStr << "/";
	tempStr << strategyMode;
	tempStr << "/";

	if(strategyMode == "limitCoords") {
		tempStr << globalViewBuilder->parProxy("gvbStrategyRemoveCoordsCoordsLimit").longValue();

	} else if (strategyMode == "limitTraffic") {
		tempStr << globalViewBuilder->parProxy("gvbStrategyRemoveCoordsTrafficLimit").longValue();

	} else if (strategyMode == "percentage") {
		tempStr << globalViewBuilder->parProxy("gvbStrategyRemoveCoordsPercentage").longValue();
	}

	return tempStr.str();
}
