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
	virtual void recursiveDelete() override;

protected:
	// TODO no need to actually store the address and the exists field per entry as both are implicit by the map
	std::map<long, NodeAddressContent*>* sortedContents_;
	size_t longestSuffix_;

	NodeAddressContent lookup(long address) override;
	void insertAtAddress(long hcAddress, std::vector<std::vector<bool>>* suffix) override;
	void insertAtAddress(long hcAddress, Node* subnode) override;
	Node* adjustSize() override;

private:
	NodeAddressContent* lookupReference(long hcAddress);
};

#endif /* LHC_H_ */
