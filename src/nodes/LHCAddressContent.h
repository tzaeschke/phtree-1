/*
 * LHCAddressContent.h
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#ifndef SRC_NODES_LHCADDRESSCONTENT_H_
#define SRC_NODES_LHCADDRESSCONTENT_H_

#include <vector>

class Node;

// TODO use union to differentiate between subnode and suffix contents and leaf ids
struct LHCAddressContent {
	bool hasSubnode;
	Node* subnode;
	std::vector<bool> suffix;
	int id;

	LHCAddressContent();
	LHCAddressContent(Node* subnode);
	LHCAddressContent(std::vector<bool>* suffix, int id);
	LHCAddressContent(std::vector<bool> suffix, int id);
};

#endif /* SRC_NODES_LHCADDRESSCONTENT_H_ */
