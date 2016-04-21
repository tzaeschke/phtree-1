/*
 * NodeAddressContent.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef NODEADDRESSCONTENT_H_
#define NODEADDRESSCONTENT_H_

#include "util/MultiDimBitset.h"

class Node;

struct NodeAddressContent {
	int id;
	bool exists;
	bool hasSubnode;
	unsigned long address;
	union {
		Node* subnode;
		MultiDimBitset* suffix;
	};
};


#endif /* NODEADDRESSCONTENT_H_ */
