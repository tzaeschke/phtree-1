/*
 * AHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_AHC_H_
#define SRC_AHC_H_

#include <vector>
#include <iostream>
#include "nodes/Node.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "nodes/AHCAddressContent.h"
#include "util/MultiDimBitset.h"

class AHC: public Node {
	friend class AHCIterator;
	friend class AssertionVisitor;
	friend class SizeVisitor;
public:
	AHC(size_t dim, size_t valueLength);
	AHC(Node& node);
	virtual ~AHC();
	NodeIterator* begin() override;
	NodeIterator* end() override;
	std::ostream& output(std::ostream& os, size_t depth) override;
	virtual void accept(Visitor* visitor, size_t depth) override;
	virtual void recursiveDelete() override;

protected:
	// TODO use arrays instead by templating the dimensions
	std::vector<AHCAddressContent> contents_;
	std::vector<MultiDimBitset> suffixes_;

	virtual NodeAddressContent lookup(unsigned long address) override;
	virtual MultiDimBitset* insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) override;
	virtual void insertAtAddress(unsigned long hcAddress, Node* subnode) override;
	virtual Node* adjustSize() override;
};

#endif /* SRC_AHC_H_ */
