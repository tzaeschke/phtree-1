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
#include "nodes/Node.h"
#include "nodes/LHCAddressContent.h"
#include "util/MultiDimBitset.h"

class LHC: public Node {
	friend class LHCIterator;
	friend class AssertionVisitor;
	friend class SizeVisitor;
public:
	LHC(size_t dim, size_t valueLength);
	virtual ~LHC();
	NodeIterator* begin() override;
	NodeIterator* end() override;
	std::ostream& output(std::ostream& os, size_t depth) override;
	virtual void accept(Visitor* visitor, size_t depth) override;
	virtual void recursiveDelete() override;
	virtual size_t getNumberOfContents() const override;

protected:
	std::map<unsigned long, LHCAddressContent> sortedContents_;
	size_t longestSuffix_;

	NodeAddressContent lookup(unsigned long address) override;
	MultiDimBitset* insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) override;
	void insertAtAddress(unsigned long hcAddress, Node* subnode) override;
	Node* adjustSize() override;

private:
	LHCAddressContent* lookupReference(unsigned long hcAddress);
};

#endif /* LHC_H_ */
