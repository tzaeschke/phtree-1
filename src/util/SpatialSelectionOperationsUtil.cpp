/*
 * SpatialSelectionOperationsUtil.cpp
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#include "SpatialSelectionOperationsUtil.h"
#include "MultiDimBitTool.h"
#include "../nodes/Node.h"
#include "../nodes/NodeAddressContent.h"

using namespace std;

#define DEBUG false

pair<bool, int> SpatialSelectionOperationsUtil::lookup(const Entry* e, Node* rootNode,
		std::vector<Node*>* visitedNodes) {

	Node* currentNode = rootNode;
	size_t depth = 0;
	size_t index = 0;

	while (true) {

		if (DEBUG)
			cout << "depth " << depth << " -> ";

		if (visitedNodes)
			visitedNodes->push_back(currentNode);

		// validate prefix
		for (size_t bit = 0; bit < currentNode->getPrefixLength(); bit++) {
			for (size_t value = 0; value < e->values_.size(); value++) {
				if (e->values_[value][index + bit]
						!= currentNode->prefix_[value][bit]) {
					if (DEBUG)
						cout << "prefix missmatch" << endl;
					return pair<bool, int>(false, 0);
				}
			}
		}

		// validate HC address
		int currentIndex = index + currentNode->getPrefixLength();
		long hcAddress = MultiDimBitTool::interleaveBits(currentIndex, e);
		NodeAddressContent content = currentNode->lookup(hcAddress);

		if (!content.exists) {
			if (DEBUG)
				cout << "HC address missmatch" << endl;
			return pair<bool, int>(false, 0);
		}

		// validate suffix or recurse
		if (content.hasSubnode) {
			++depth;
			index = currentIndex + 1;
			currentNode = content.subnode;
		} else {
			for (size_t bit = 0; bit < currentNode->getSuffixSize(content);
					bit++) {
				for (size_t value = 0; value < e->values_.size(); value++) {
					if (e->values_[value][currentIndex + 1 + bit]
							!= (*content.suffix)[value][bit]) {
						if (DEBUG)
							cout << "suffix missmatch" << endl;
						return pair<bool, int>(false, 0);
					}
				}
			}

			if (DEBUG)
				cout << "found" << endl;
			return pair<bool, int>(true, content.id);
		}
	}
}
