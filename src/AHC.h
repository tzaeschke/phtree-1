/*
 * AHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_AHC_H_
#define SRC_AHC_H_

#include <vector>
#include "Node.h"

class AHC: public Node {
	friend class AHCIterator;
public:
	AHC(size_t dim, size_t valueLength);
	AHC(size_t dim, size_t valueLength, Node& node);
	virtual ~AHC();
	NodeIterator begin();
	NodeIterator end();

protected:
	vector<bool> filled_;
	vector<bool> hasSubnode_;
	vector<Node *> subnodes_;
	// entry -> value -> bit
	vector<vector<vector<bool>>> suffixes_;

	NodeAddressContent lookup(long address);
	void insertAtAddress(long hcAddress, vector<vector<bool>>* suffix);
	void insertAtAddress(long hcAddress, Node* subnode);
	Node* adjustSize();
	ostream& output(ostream& os, size_t depth);
};

#endif /* SRC_AHC_H_ */
