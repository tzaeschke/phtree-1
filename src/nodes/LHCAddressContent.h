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

// TODO use union to differentiate between subnode and suffix contents
struct LHCAddressContent {
	bool hasSubnode;
	Node* subnode;
	std::vector<std::vector<bool>> suffix;

	LHCAddressContent();
	LHCAddressContent(Node* subnode);
	LHCAddressContent(std::vector<std::vector<bool>>* suffix);
	LHCAddressContent(std::vector<std::vector<bool>> suffix);
};

#endif /* SRC_NODES_LHCADDRESSCONTENT_H_ */
