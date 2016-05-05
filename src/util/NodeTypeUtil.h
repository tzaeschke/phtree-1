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
	static Node<DIM>* buildNode(size_t prefixBlocks, size_t nDirectInserts) {
		switch (prefixBlocks) {
		case 0: return determineNodeType<DIM, 0>(nDirectInserts);
		case 1: return determineNodeType<DIM, 1>(nDirectInserts);
		case 2: return determineNodeType<DIM, 2>(nDirectInserts);
		case 3: return determineNodeType<DIM, 3>(nDirectInserts);
		case 4: return determineNodeType<DIM, 4>(nDirectInserts);
		case 5: return determineNodeType<DIM, 5>(nDirectInserts);
		case 6: return determineNodeType<DIM, 6>(nDirectInserts);
		default: throw "Only supports up to 6 prefix blocks right now.";
		}
	}

private:
	template <unsigned int DIM, unsigned int PREF_BLOCKS>
	inline static Node<DIM>* determineNodeType(size_t nDirectInserts) {
		assert (nDirectInserts <= 2);
		// TODO determine node type dynamically depending on dim and #inserts
		return new LHC<DIM, PREF_BLOCKS>();
	}
};

#endif /* SRC_UTIL_NODETYPEUTIL_H_ */
