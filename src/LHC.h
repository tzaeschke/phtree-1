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
#include "Node.h"

class LHC: public Node {
	friend class LHCIterator;
	friend class AssertionVisitor;
public:
	LHC(size_t dim, size_t valueLength);
	virtual ~LHC();
	NodeIterator* begin() override;
	NodeIterator* end() override;
	std::ostream& output(std::ostream& os, size_t depth) override;
	virtual void accept(Visitor* visitor, size_t depth) override;

protected:
	std::map<long, NodeAddressContent*>* sortedContents_;
	size_t longestSuffix_;
	long highestAddress;

	NodeAddressContent* lookup(long address) override;
	void insertAtAddress(long hcAddress, std::vector<std::vector<bool>>* suffix) override;
	void insertAtAddress(long hcAddress, Node* subnode) override;
	Node* adjustSize() override;
};

#endif /* LHC_H_ */
