/*
 * AreaDataContainer.h
 *
 *  Created on: 10.08.2010
 *      Author: user
 */

#ifndef AREADATACONTAINER_H_
#define AREADATACONTAINER_H_


#include <CoordBasedRouting.h>

class AreaDataContainer {

public:
	AreaDataContainer();
	virtual ~AreaDataContainer();

	AP CBRAreaPool;
};

#endif /* AREADATACONTAINER_H_ */
