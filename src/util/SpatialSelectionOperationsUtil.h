/*
 * SpatialSelectionOperationsUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_
#define SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_

#include <vector>

template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class Entry;

template <unsigned int DIM, unsigned int WIDTH>
class SpatialSelectionOperationsUtil {
public:
	static std::pair<bool, int> lookup(const Entry<DIM, WIDTH>* e,
			Node<DIM>* rootNode, std::vector<Node<DIM>*>* visitedNodes);
};

#include <assert.h>
#include "nodes/Node.h"
#include "nodes/NodeAddressContent.h"

using namespace std;


template <unsigned int DIM, unsigned int WIDTH>
pair<bool, int> SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(const Entry<DIM, WIDTH>* e, Node<DIM>* rootNode,
		vector<Node<DIM>*>* visitedNodes) {

	Node<DIM>* currentNode = rootNode;
	size_t depth = 0;
	size_t index = 0;
	NodeAddressContent<DIM> content;

	while (true) {

		#ifdef PRINT
			cout << "depth " << depth << " -> ";
		#endif

		if (visitedNodes)
			visitedNodes->push_back(currentNode);

		// validate prefix
		// TODO move to multi dim bit util
		pair<bool, size_t> comp = MultiDimBitset<DIM>::compare(e->values_, DIM * WIDTH,
				index, index + currentNode->getPrefixLength(),
				currentNode->getPrefixStartBlock(), currentNode->getPrefixLength() * DIM);
		if (!comp.first) {
			#ifdef PRINT
				cout << "prefix mismatch" << endl;
			#endif
			return pair<bool, int>(false, 0);
		}

		// validate HC address
		size_t currentIndex = index + currentNode->getPrefixLength();
		unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(e->values_, currentIndex, DIM * WIDTH);
		currentNode->lookup(hcAddress, content);

		if (!content.exists) {
			#ifdef PRINT
				cout << "HC address mismatch" << endl;
			#endif
			return pair<bool, int>(false, 0);
		}

		// validate suffix or recurse
		if (content.hasSubnode) {
			++depth;
			index = currentIndex + 1;
			currentNode = content.subnode;
		} else {
			const size_t suffixBits = DIM * (WIDTH - currentIndex - 1);
			comp = MultiDimBitset<DIM>::compare(e->values_, DIM * WIDTH,
							currentIndex + 1, WIDTH,
							content.suffixStartBlock, suffixBits);
			if (!comp.first) {
				#ifdef PRINT
					cout << "suffix mismatch" << endl;
				#endif
				return pair<bool, int>(false, 0);
			}

			#ifdef PRINT
				cout << "found" << endl;
			#endif

			return pair<bool, int>(true, content.id);
		}
	}
}

#endif /* SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_ */
