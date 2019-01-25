/*
 * sendStrategyFactory.h
 *
 *  Created on: 08.03.2010
 *      Author: user
 */

#ifndef SENDSTRATEGYFACTORY_H_
#define SENDSTRATEGYFACTORY_H_

#include <string>

#include <AbstractSendStrategy.h>
#include <StrategyTreeTest.h>
#include <StrategySendAll.h>
#include <StrategyRegions.h>
#include <StrategyRemoveRandom.h>
#include <StrategyRemoveInaccurate.h>
#include <StrategySimplifyCoords.h>


class SendStrategyFactory {
public:
	SendStrategyFactory();
	virtual ~SendStrategyFactory();

	static AbstractSendStrategy *getSendStrategyInstance(const std::string sendStrategyName);
};

#endif /* SENDSTRATEGYFACTORY_H_ */
