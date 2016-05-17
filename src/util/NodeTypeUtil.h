/*
 * NodeTypeUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_NODETYPEUTIL_H_
#define SRC_UTIL_NODETYPEUTIL_H_

#include <cstdint>
#include "nodes/LHC.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class NodeTypeUtil {
public:
	static Node<DIM>* buildNode(size_t prefixBits, size_t nDirectInserts) {
		const size_t prefixBlocks = (prefixBits > 0)? 1 + ((prefixBits - 1) / sizeof (unsigned long)) : 0;
		switch (prefixBlocks) {
		case 0: return determineNodeType<0>(prefixBits, nDirectInserts);
		case 1: return determineNodeType<1>(prefixBits, nDirectInserts);
		case 2: return determineNodeType<2>(prefixBits, nDirectInserts);
		case 3: return determineNodeType<3>(prefixBits, nDirectInserts);
		case 4: return determineNodeType<4>(prefixBits, nDirectInserts);
		case 5: return determineNodeType<5>(prefixBits, nDirectInserts);
		case 6: return determineNodeType<6>(prefixBits, nDirectInserts);
		default: throw "Only supports up to 6 prefix blocks right now.";
		}
	}

	static Node<DIM>* copyWithoutPrefix(size_t newPrefixBits, Node<DIM>* nodeToCopy) {
		// TODO make more efficient by not using iterators and a bulk insert
		size_t nDirectInsert = nodeToCopy->getNumberOfContents();
		Node<DIM>* copy = buildNode(newPrefixBits, nDirectInsert);
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

	static Node<DIM>* copyIntoLargerNode(size_t newNContents, Node<DIM>* nodeToCopy) {
		// TODO make more efficient by not using iterators and a bulk insert
		Node<DIM>* copy = buildNode(nodeToCopy->getPrefixLength() * DIM, newNContents);
		MultiDimBitset<DIM>::duplicateHighestBits(nodeToCopy->getFixPrefixStartBlock(),
				nodeToCopy->getPrefixLength() * DIM, nodeToCopy->getPrefixLength(),
				copy->getPrefixStartBlock());
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

	template <unsigned int PREF_BLOCKS>
	inline static Node<DIM>* determineNodeType(size_t prefixBits, size_t nDirectInserts) {
		assert (nDirectInserts > 0);
		const size_t prefixLength = prefixBits / DIM;
		// TODO use threshold depending on which node is smaller
		const double switchTypeAtLoadRatio = 0.75;
		if (float(nDirectInserts) / (1 << DIM) < switchTypeAtLoadRatio) {
			return determineLhcSize<PREF_BLOCKS>(prefixLength, nDirectInserts);
		} else {
			return new AHC<DIM, PREF_BLOCKS>(prefixLength);
		}
	}

	template <unsigned int PREF_BLOCKS>
	inline static Node<DIM>* determineLhcSize(size_t prefixLength, size_t nDirectInserts) {

		assert (nDirectInserts > 0);
		const float insertToRatio = float(nDirectInserts) / (1 << DIM);
		assert (0 < insertToRatio && insertToRatio < 1);

		if (nDirectInserts < 3) {
			return new LHC<DIM, PREF_BLOCKS, 2>(prefixLength);
		} else if (insertToRatio < 0.1) {
			return new LHC<DIM, PREF_BLOCKS,  1 + 10 * (1 << DIM) / 100>(prefixLength);
		} else if (insertToRatio < 0.25) {
			return new LHC<DIM, PREF_BLOCKS,  1 + 25 * (1 << DIM) / 100>(prefixLength);
		} else if (insertToRatio < 0.5) {
			return new LHC<DIM, PREF_BLOCKS,  1 + 50 * (1 << DIM) / 100>(prefixLength);
		} else if (insertToRatio < 0.75) {
			return new LHC<DIM, PREF_BLOCKS,  1 + 75 * (1 << DIM) / 100>(prefixLength);
		} else {
			return new LHC<DIM, PREF_BLOCKS, (1 << DIM)>(prefixLength);
		}
	}
};

#endif /* SRC_UTIL_NODETYPEUTIL_H_ */
