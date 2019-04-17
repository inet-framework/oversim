/*
 * sendStrategyFactory.h
 *
 *  Created on: 08.03.2010
 *      Author: user
 */

#ifndef SENDSTRATEGYFACTORY_H_
#define SENDSTRATEGYFACTORY_H_

#include <string>

#include "common/cbr/globalViewBuilder/AbstractSendStrategy.h"
#include "common/cbr/globalViewBuilder/StrategyTreeTest.h"
#include "common/cbr/globalViewBuilder/StrategySendAll.h"
#include "common/cbr/globalViewBuilder/StrategyRegions.h"
#include "common/cbr/globalViewBuilder/StrategyRemoveRandom.h"
#include "common/cbr/globalViewBuilder/StrategyRemoveInaccurate.h"
#include "common/cbr/globalViewBuilder/StrategySimplifyCoords.h"


class SendStrategyFactory {
public:
	SendStrategyFactory();
	virtual ~SendStrategyFactory();

	static AbstractSendStrategy *getSendStrategyInstance(const std::string sendStrategyName);
};

#endif /* SENDSTRATEGYFACTORY_H_ */
