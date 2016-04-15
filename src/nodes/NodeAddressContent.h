/*
 * NodeAddressContent.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef NODEADDRESSCONTENT_H_
#define NODEADDRESSCONTENT_H_

#include "boost/dynamic_bitset.hpp"

class Node;

// TODO use union to differentiate between subnode and suffix contents
// TODO combine exists and hasSubnode in single byte
struct NodeAddressContent {
	unsigned long address;
	bool exists;
	bool hasSubnode;
	Node* subnode;
	boost::dynamic_bitset<>* suffix;
	int id;
};


#endif /* NODEADDRESSCONTENT_H_ */
