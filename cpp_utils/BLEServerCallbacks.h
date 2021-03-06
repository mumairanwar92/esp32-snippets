/*
 * BLEServerCallbacks.h
 *
 *  Created on: Jul 4, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_BLESERVERCALLBACKS_H_
#define COMPONENTS_CPP_UTILS_BLESERVERCALLBACKS_H_
#include "BLEServer.h"
class BLEServer;

class BLEServerCallbacks {
public:
	BLEServerCallbacks();
	virtual ~BLEServerCallbacks();
	virtual void onConnect(BLEServer *pServer);
	virtual void onDisconnect(BLEServer *pServer);
};

#endif /* COMPONENTS_CPP_UTILS_BLESERVERCALLBACKS_H_ */
