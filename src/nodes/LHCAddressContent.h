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

// TODO use union to differentiate between subnode and suffix contents and leaf ids
struct LHCAddressContent {
	bool hasSubnode;
	Node* subnode;
	MultiDimBitset suffix;
	int id;

	LHCAddressContent();
	LHCAddressContent(const LHCAddressContent &other);
	LHCAddressContent(Node* subnode);
	LHCAddressContent(MultiDimBitset* suffix, int id);
	LHCAddressContent(MultiDimBitset suffix, int id);
};

#endif /* SRC_NODES_LHCADDRESSCONTENT_H_ */
