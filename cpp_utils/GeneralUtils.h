/*
 * GeneralUtils.h
 *
 *  Created on: May 20, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_GENERALUTILS_H_
#define COMPONENTS_CPP_UTILS_GENERALUTILS_H_
#include <stdint.h>
#include <string>

/**
 * @brief General utilities.
 */
class GeneralUtils {
public:
	GeneralUtils();
	virtual ~GeneralUtils();
	static void hexDump(uint8_t *pData, uint32_t length);
	static std::string ipToString(uint8_t *ip);
};

#endif /* COMPONENTS_CPP_UTILS_GENERALUTILS_H_ */
