/*
 * LHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef LHC_H_
#define LHC_H_

#include "Node.h"
#include <map>
#include <vector>

class LHC: public Node {
public:
	LHC(size_t dim, size_t valueLength);
	virtual ~LHC();

protected:
	map<long, Node::NodeAddressContent*>* sortedContents_;

	Node::NodeAddressContent lookup(long address);
	void insertAtAddress(long hcAddress, vector<vector<bool>>* suffix);
	void insertAtAddress(long hcAddress, Node* subnode);
	ostream& output(ostream& os, size_t depth);
};

#endif /* LHC_H_ */
