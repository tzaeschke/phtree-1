/*
 * SpatialSelectionOperationsUtil.cpp
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#include <assert.h>
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
	size_t dim = e->dim_;

	while (true) {

		if (DEBUG)
			cout << "depth " << depth << " -> ";

		if (visitedNodes)
			visitedNodes->push_back(currentNode);

		// validate prefix
		// TODO move to multi dim bit util
		for (size_t i = 0; i < currentNode->prefix_.size(); ++i) {
			if (currentNode->prefix_.at(i) != e->values_.at(dim * index + i)) {
				if (DEBUG)
					cout << "prefix missmatch" << endl;
				return pair<bool, int>(false, 0);
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
			// TODO move to multi dim bit util
			assert (content.suffix->size() == e->values_.size() - dim * (currentIndex + 1));
			for (size_t i = 0; i < content.suffix->size(); ++i) {
				if ((*content.suffix).at(i) != e->values_.at(dim * (currentIndex + 1) + i)) {
					if (DEBUG)
						cout << "suffix missmatch" << endl;
					return pair<bool, int>(false, 0);
				}
			}

			if (DEBUG)
				cout << "found" << endl;
			return pair<bool, int>(true, content.id);
		}
	}
}
