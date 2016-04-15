/*
 * DynamicNodeOperationsUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_
#define SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_

#include "nodes/NodeAddressContent.h"

class Entry;
class Node;

class DynamicNodeOperationsUtil {
public:

	static Node* insert(const Entry* e, Node* rootNode, size_t dim, size_t bitLength);

private:
	static inline void createSubnodeWithExistingSuffix(size_t dim, size_t bitLength,
			size_t currentIndex, Node* currentNode, NodeAddressContent content,
			const Entry* entry);
	static inline Node* insertSuffix(size_t dim, size_t currentIndex, size_t hcAddress,
			Node* currentNode,  const Entry* entry);
	static inline void splitSubnodePrefix(size_t dim, size_t bitLength,
			size_t currentIndex, size_t differentAtIndex, Node* currentNode,
			NodeAddressContent content, const Entry* entry);
};

#endif /* SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_ */
