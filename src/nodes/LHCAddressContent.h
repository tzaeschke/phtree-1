/*
 * LHCAddressContent.h
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#ifndef SRC_NODES_LHCADDRESSCONTENT_H_
#define SRC_NODES_LHCADDRESSCONTENT_H_

#include "util/MultiDimBitset.h"

class Node;

struct LHCAddressContent {
	bool hasSubnode;
	int id;
	Node* subnode;
	MultiDimBitset suffix;

	LHCAddressContent();
	LHCAddressContent(const LHCAddressContent &other);
	LHCAddressContent(Node* subnode);
	LHCAddressContent(const MultiDimBitset* suffix, int id);
	LHCAddressContent(const size_t dim, int id);
};

#endif /* SRC_NODES_LHCADDRESSCONTENT_H_ */
