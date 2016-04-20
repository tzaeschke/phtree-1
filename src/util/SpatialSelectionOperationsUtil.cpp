/*
 * SpatialSelectionOperationsUtil.cpp
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#include <assert.h>
#include "util/SpatialSelectionOperationsUtil.h"
#include "nodes/Node.h"
#include "nodes/NodeAddressContent.h"

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
		pair<bool, size_t> comp = e->values_.compareTo(index,
				index + currentNode->prefix_.getBitLength(),
				currentNode->prefix_);
		if (!comp.first) {
			if (DEBUG)
				cout << "prefix missmatch" << endl;
			return pair<bool, int>(false, 0);
		}

		// validate HC address
		size_t currentIndex = index + currentNode->getPrefixLength();
		unsigned long hcAddress = e->values_.interleaveBits(currentIndex);
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
			pair<bool, size_t> comp = e->values_.compareTo(currentIndex + 1, currentIndex + 1 + content.suffix->getBitLength(), *content.suffix);
			if (!comp.first) {
				if (DEBUG)
					cout << "suffix missmatch" << endl;
				return pair<bool, int>(false, 0);
			}

			if (DEBUG)
				cout << "found" << endl;
			return pair<bool, int>(true, content.id);
		}
	}
}
