/*
 * NodeAddressContent.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef NODEADDRESSCONTENT_H_
#define NODEADDRESSCONTENT_H_

#include <vector>

class Node;

// TODO use union to differentiate between subnode and suffix contents
struct NodeAddressContent {
	unsigned long address;
	bool exists;
	bool hasSubnode;
	Node* subnode;
	std::vector<bool>* suffix;
	int id;
};


#endif /* NODEADDRESSCONTENT_H_ */
