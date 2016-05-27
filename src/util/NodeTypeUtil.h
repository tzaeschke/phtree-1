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
		const size_t prefixBlocks = (prefixBits > 0)? 1 + ((prefixBits - 1) / (8 * sizeof (unsigned long))) : 0;
		switch (prefixBlocks) {
		case 0: return determineNodeType<0>(prefixBits, nDirectInserts);
		case 1: return determineNodeType<1>(prefixBits, nDirectInserts);
		case 2: return determineNodeType<2>(prefixBits, nDirectInserts);
		case 3: return determineNodeType<3>(prefixBits, nDirectInserts);
		case 4: return determineNodeType<4>(prefixBits, nDirectInserts);
		case 5: return determineNodeType<5>(prefixBits, nDirectInserts);
		case 6: return determineNodeType<6>(prefixBits, nDirectInserts);
		case 7: return determineNodeType<7>(prefixBits, nDirectInserts);
		case 8: return determineNodeType<8>(prefixBits, nDirectInserts);
		case 9: return determineNodeType<9>(prefixBits, nDirectInserts);
		case 10: return determineNodeType<10>(prefixBits, nDirectInserts);
		case 11: return determineNodeType<11>(prefixBits, nDirectInserts);
		case 12: return determineNodeType<12>(prefixBits, nDirectInserts);
		default: throw runtime_error("Only supports up to 12 prefix blocks right now.");
		}
	}

	static Node<DIM>* copyWithoutPrefix(size_t newPrefixBits, const Node<DIM>* nodeToCopy) {
		size_t nDirectInsert = nodeToCopy->getNumberOfContents();
		Node<DIM>* copy = buildNode(newPrefixBits, nDirectInsert);
		copyContents(*nodeToCopy, *copy);
		return copy;
	}

	static Node<DIM>* copyIntoLargerNode(size_t newNContents, const Node<DIM>* nodeToCopy) {
		// TODO make more efficient by not using iterators and a bulk insert
		Node<DIM>* copy = buildNode(nodeToCopy->getPrefixLength() * DIM, newNContents);
		MultiDimBitset<DIM>::duplicateHighestBits(nodeToCopy->getFixPrefixStartBlock(),
				nodeToCopy->getPrefixLength() * DIM, nodeToCopy->getPrefixLength(),
				copy->getPrefixStartBlock());
		copyContents(*nodeToCopy, *copy);
		return copy;
	}


private:

	inline static void copyContents(const Node<DIM>& from, Node<DIM>& to) {
		// TODO make more efficient by not using iterators and a bulk insert
		NodeIterator<DIM>* it;
		NodeIterator<DIM>* endIt = from.end();
		for (it = from.begin(); (*it) != *endIt; ++(*it)) {
			NodeAddressContent<DIM> content = *(*it);
			if (content.hasSubnode) {
				to.insertAtAddress(content.address, content.subnode);
			} else if (content.directlyStoredSuffix) {
				to.insertAtAddress(content.address, content.suffix, content.id);
			} else {
				to.insertAtAddress(content.address, content.suffixStartBlock, content.id);
			}
		}

		delete it;
		delete endIt;
	}

	template <unsigned int PREF_BLOCKS>
	inline static Node<DIM>* determineNodeType(size_t prefixBits, size_t nDirectInserts) {
		assert (nDirectInserts > 0);
		const size_t prefixLength = prefixBits / DIM;
		// TODO use threshold depending on which node is smaller
		const double switchTypeAtLoadRatio = 0.75;
		if (float(nDirectInserts) / (1uL << DIM) < switchTypeAtLoadRatio) {
			return determineLhcSize<PREF_BLOCKS>(prefixLength, nDirectInserts);
		} else {
			return new AHC<DIM, PREF_BLOCKS>(prefixLength);
		}
	}

	template <unsigned int PREF_BLOCKS>
	inline static Node<DIM>* determineLhcSize(size_t prefixLength, size_t nDirectInserts) {

		assert (nDirectInserts > 0);
		const float insertToRatio = float(nDirectInserts) / (1u << DIM);
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
