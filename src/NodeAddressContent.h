/*
 * NodeAddressContent.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef NODEADDRESSCONTENT_H_
#define NODEADDRESSCONTENT_H_

#include "Node.h"

// TODO use union to differentiate between subnode and suffix contents
struct NodeAddressContent {
	long address;
	bool contained;
	bool hasSubnode;
	Node* subnode;
	std::vector<std::vector<bool>>* suffix;
	};


#endif /* NODEADDRESSCONTENT_H_ */
