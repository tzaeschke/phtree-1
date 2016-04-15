/*
 * AHCAddressContent.h
 *
 *  Created on: Apr 14, 2016
 *      Author: max
 */

#ifndef SRC_NODES_AHCADDRESSCONTENT_H_
#define SRC_NODES_AHCADDRESSCONTENT_H_

#include <vector>

class Node;

struct AHCAddressContent {
	AHCAddressContent();
	AHCAddressContent(Node* subnode);
	AHCAddressContent(int id);

	bool filled;
	bool hasSubnode;
	union {
		Node* subnode;
		struct {
			int id;
		};
	};
};

#endif /* SRC_NODES_AHCADDRESSCONTENT_H_ */
