/*
 * NodeTypeUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_NODETYPEUTIL_H_
#define SRC_UTIL_NODETYPEUTIL_H_

#include "../nodes/LHC.h"

class Node;

class NodeTypeUtil {
public:
	static Node* determineNodeType(size_t dim, size_t valueLength, size_t nDirectInserts) {
		// TODO determine node type dynamically depending on dim and #inserts
		LHC* node = new LHC(dim, valueLength);
		return node;
	}
};

#endif /* SRC_UTIL_NODETYPEUTIL_H_ */
