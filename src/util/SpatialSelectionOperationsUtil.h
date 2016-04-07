/*
 * SpatialSelectionOperationsUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_
#define SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_

#include <vector>

class Node;
class Entry;

class SpatialSelectionOperationsUtil {
public:
	static std::pair<bool, int> lookup(const Entry* e, Node* rootNode, std::vector<Node*>* visitedNodes);
};

#endif /* SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_ */
