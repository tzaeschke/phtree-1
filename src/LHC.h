/*
 * LHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef LHC_H_
#define LHC_H_

#include <map>
#include <vector>
#include "Node.cpp"

class LHC: public Node {
	friend class LHCIterator;
public:
	LHC(size_t dim, size_t valueLength);
	virtual ~LHC();

	NodeIterator begin();
	NodeIterator end();

protected:
	map<long, NodeAddressContent*>* sortedContents_;
	size_t longestSuffix_;
	long highestAddress;

	NodeAddressContent lookup(long address);
	void insertAtAddress(long hcAddress, vector<vector<bool>>* suffix);
	void insertAtAddress(long hcAddress, Node* subnode);
	Node* adjustSize();
	ostream& output(ostream& os, size_t depth);
};

#endif /* LHC_H_ */
