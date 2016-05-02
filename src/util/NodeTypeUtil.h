/*
 * NodeTypeUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_NODETYPEUTIL_H_
#define SRC_UTIL_NODETYPEUTIL_H_

#include "nodes/LHC.h"

template <unsigned int DIM>
class Node;

class NodeTypeUtil {
public:
	template <unsigned int DIM>
	static Node<DIM>* determineNodeType(size_t valueLength, size_t nDirectInserts) {
		// TODO determine node type dynamically depending on dim and #inserts
		LHC<DIM>* node = new LHC<DIM>(valueLength);
		return node;
	}
};

#endif /* SRC_UTIL_NODETYPEUTIL_H_ */
