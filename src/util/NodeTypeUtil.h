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
	static Node<DIM>* buildNode(size_t prefixBits, size_t nDirectInserts) {
		const size_t prefixBlocks = (prefixBits > 0)? 1 + ((prefixBits - 1) / sizeof (unsigned long)) : 0;
		switch (prefixBlocks) {
		case 0: return determineNodeType<DIM, 0>(prefixBits, nDirectInserts);
		case 1: return determineNodeType<DIM, 1>(prefixBits, nDirectInserts);
		case 2: return determineNodeType<DIM, 2>(prefixBits, nDirectInserts);
		case 3: return determineNodeType<DIM, 3>(prefixBits, nDirectInserts);
		case 4: return determineNodeType<DIM, 4>(prefixBits, nDirectInserts);
		case 5: return determineNodeType<DIM, 5>(prefixBits, nDirectInserts);
		case 6: return determineNodeType<DIM, 6>(prefixBits, nDirectInserts);
		default: throw "Only supports up to 6 prefix blocks right now.";
		}
	}

	template <unsigned int DIM>
	static Node<DIM>* copyWithoutPrefix(size_t newPrefixBits, Node<DIM>* nodeToCopy) {
		// TODO make more efficient by not using iterators and a bulk insert
		size_t nDirectInsert = nodeToCopy->getNumberOfContents();
		Node<DIM>* copy = buildNode<DIM>(newPrefixBits, nDirectInsert);
		NodeIterator<DIM>* it;
		NodeIterator<DIM>* endIt = nodeToCopy->end();
		for (it = nodeToCopy->begin(); (*it) != *endIt; ++(*it)) {
			NodeAddressContent<DIM> content = *(*it);
			if (content.hasSubnode) {
				copy->insertAtAddress(content.address, content.subnode);
			} else {
				copy->insertAtAddress(content.address, content.suffixStartBlock, content.id);
			}
		}

		delete it;
		delete endIt;
		return copy;
	}

private:
	template <unsigned int DIM, unsigned int PREF_BLOCKS>
	inline static Node<DIM>* determineNodeType(size_t prefixBits, size_t nDirectInserts) {

		const size_t prefixLength = prefixBits / DIM;

		// TODO find more precise threshold depending on AHC and LHC representation!
		const size_t n = nDirectInserts;
		const size_t k = DIM;
		const double conversionThreshold = double(1uL << k) / (k);
		if (n < conversionThreshold) {
			return new LHC<DIM, PREF_BLOCKS>(prefixLength);
		} else {
			return new AHC<DIM, PREF_BLOCKS>(prefixLength);
		}
	}
};

#endif /* SRC_UTIL_NODETYPEUTIL_H_ */
