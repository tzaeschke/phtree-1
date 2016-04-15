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
#include "boost/dynamic_bitset.hpp"

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
	std::vector<boost::dynamic_bitset<>> suffixes_;

	virtual NodeAddressContent lookup(unsigned long address) override;
	virtual void insertAtAddress(unsigned long hcAddress, boost::dynamic_bitset<>* suffix, int id) override;
	virtual void insertAtAddress(unsigned long hcAddress, Node* subnode) override;
	virtual Node* adjustSize() override;
};

#endif /* SRC_AHC_H_ */
