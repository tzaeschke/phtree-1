/*
 * Node.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include "Entry.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "util/MultiDimBitset.h"

class Visitor;
class RangeQueryIterator;

class Node {
	// TODO remove friends and use getters and setters
	friend class NodeIterator;
	friend std::ostream& operator<<(std::ostream& os, Node& node);
	friend class RangeQueryIterator;
	friend class DynamicNodeOperationsUtil;
	friend class SpatialSelectionOperationsUtil;
	friend class SizeVisitor;
public:

	Node(size_t dim, size_t valueLength);
	Node(Node* other);
	virtual ~Node();
	RangeQueryIterator* rangeQuery(const Entry* lowerLeft, const Entry* upperRight, size_t depth, size_t index);

	virtual std::ostream& output(std::ostream& os, size_t depth) = 0;
	virtual NodeIterator* begin() = 0;
	virtual NodeIterator* end() = 0;
	virtual void accept(Visitor* visitor, size_t depth);
	virtual void recursiveDelete() = 0;

protected:
	size_t dim_;
	size_t valueLength_;
	// value -> bit
	MultiDimBitset prefix_;

	size_t getSuffixSize(NodeAddressContent) const;
	size_t getPrefixLength() const;


	virtual NodeAddressContent lookup(unsigned long address) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, MultiDimBitset* suffix, int id) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, Node* subnode) = 0;
	virtual Node* adjustSize() = 0;
};

#endif /* SRC_NODE_H_ */
