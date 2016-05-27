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
	static std::pair<bool, int> lookup(const Entry<DIM, WIDTH>& e,
			const Node<DIM>* rootNode,
			std::vector<std::pair<unsigned long, const Node<DIM>*>>* visitedNodes);
};

#include <assert.h>
#include "nodes/Node.h"
#include "nodes/NodeAddressContent.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
pair<bool, int> SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(
		const Entry<DIM, WIDTH>& e,
		const Node<DIM>* rootNode,
		vector<pair<unsigned long, const Node<DIM>*>>* visitedNodes) {

	const Node<DIM>* currentNode = rootNode;
	unsigned long lastHcAddress = 0;
	size_t index = 0;
	NodeAddressContent<DIM> content;

	while (true) {

		#ifdef PRINT
			cout << "depth " << depth << " -> ";
		#endif

		if (visitedNodes)
			visitedNodes->push_back(pair<unsigned long, const Node<DIM>*>(lastHcAddress, currentNode));

		const size_t prefixLength = currentNode->getPrefixLength();
		if (prefixLength > 0) {
			// validate prefix
			const pair<bool, size_t> prefixComp = MultiDimBitset<DIM>::compare(e.values_, DIM * WIDTH,
					index, index + prefixLength,
					currentNode->getFixPrefixStartBlock(), prefixLength * DIM);

			if (!prefixComp.first) {
				#ifdef PRINT
					cout << "prefix mismatch" << endl;
				#endif
				return pair<bool, int>(false, 0);
			}
		}


		// validate HC address
		index += prefixLength;
		const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(e.values_, index, DIM * WIDTH);
		currentNode->lookup(hcAddress, content);

		if (!content.exists) {
			#ifdef PRINT
				cout << "HC address mismatch" << endl;
			#endif
			return pair<bool, int>(false, 0);
		}

		if (content.hasSubnode) {
			// recurse
			++index;
			currentNode = content.subnode;
			lastHcAddress = content.address;
		} else {
			const size_t suffixBits = DIM * (WIDTH - index - 1);
			if (suffixBits > 0) {
				// validate suffix
				const pair<bool, size_t> suffixComp = MultiDimBitset<DIM>::compare(e.values_, DIM * WIDTH,
								index + 1, WIDTH,
								content.suffixStartBlock, suffixBits);
				if (!suffixComp.first) {
					#ifdef PRINT
						cout << "suffix mismatch" << endl;
					#endif
					return pair<bool, int>(false, 0);
				}
			}

			#ifdef PRINT
				cout << "found" << endl;
			#endif

			return pair<bool, int>(true, content.id);
		}
	}
}

#endif /* SRC_UTIL_SPATIALSELECTIONOPERATIONSUTIL_H_ */
