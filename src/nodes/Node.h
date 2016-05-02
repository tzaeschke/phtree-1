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

template <unsigned int DIM>
class Visitor;
template <unsigned int DIM>
class RangeQueryIterator;
template <unsigned int DIM>
class SizeVisitor;
template <unsigned int DIM>
class PrefixSharingVisitor;
template <unsigned int DIM>
class DynamicNodeOperationsUtil;

template <unsigned int DIM>
class Node {
	// TODO remove friends and use getters and setters
	friend class NodeIterator<DIM>;
	template <unsigned int D>
	friend std::ostream& operator<<(std::ostream& os, Node<D>& node);
	friend class RangeQueryIterator<DIM>;
	friend class DynamicNodeOperationsUtil<DIM>;
	friend class SpatialSelectionOperationsUtil;
	friend class SizeVisitor<DIM>;
	friend class PrefixSharingVisitor<DIM>;
public:

	Node(size_t valueLength);
	Node(Node* other);
	virtual ~Node();
	RangeQueryIterator<DIM>* rangeQuery(const Entry<DIM>* lowerLeft, const Entry<DIM>* upperRight, size_t depth, size_t index);

	virtual std::ostream& output(std::ostream& os, size_t depth) = 0;
	virtual NodeIterator<DIM>* begin() = 0;
	virtual NodeIterator<DIM>* end() = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth);
	virtual void recursiveDelete() = 0;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;

protected:
	const size_t valueLength_;
	// value -> bit
	MultiDimBitset<DIM> prefix_;

	size_t getSuffixSize(NodeAddressContent<DIM>) const;
	size_t getPrefixLength() const;


	virtual NodeAddressContent<DIM> lookup(unsigned long address) = 0;
	virtual MultiDimBitset<DIM>* insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) = 0;
	virtual Node<DIM>* adjustSize() = 0;
};

#include <assert.h>
#include <stdexcept>
#include "nodes/LHC.h"
#include "util/SpatialSelectionOperationsUtil.h"
#include "iterators/RangeQueryIterator.h"

using namespace std;

#define DEBUG false

template <unsigned int DIM>
Node<DIM>::Node(size_t valueLength) : valueLength_(valueLength), prefix_() {
}

template <unsigned int DIM>
Node<DIM>::Node(Node* other) : valueLength_(other->valueLength_), prefix_(other->prefix_) {
}

template <unsigned int DIM>
Node<DIM>::~Node() {
	prefix_.clear();
}

template <unsigned int DIM>
RangeQueryIterator<DIM>* Node<DIM>::rangeQuery(const Entry<DIM>* lowerLeft, const Entry<DIM>* upperRight, size_t depth, size_t index) {
	vector<Node<DIM>*>* visitedNodes = new vector<Node<DIM>*>();
	SpatialSelectionOperationsUtil::lookup(lowerLeft, this, visitedNodes);
	RangeQueryIterator<DIM>* iterator = new RangeQueryIterator<DIM>(visitedNodes, DIM, valueLength_, lowerLeft, upperRight);
	return iterator;
}

template <unsigned int DIM>
size_t Node<DIM>::getSuffixSize(NodeAddressContent<DIM> content) const {
	if (content.hasSubnode) {
		return 0;
	} else {
		return content.suffix->size() / DIM;
	}
}

template <unsigned int DIM>
size_t Node<DIM>::getPrefixLength() const {
	return prefix_.size() / DIM;
}

template <unsigned int DIM>
void Node<DIM>::accept(Visitor<DIM>* visitor, size_t depth) {
	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = this->end();
	for (it = this->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			content.subnode->accept(visitor, depth + 1);
		}
	}

	delete it;
	delete endIt;
}

template <unsigned int DIM>
ostream& Node<DIM>::output(ostream& os, size_t depth) {
	return os << "subclass should overwrite this";
}

template <unsigned int D>
ostream& operator <<(ostream& os, Node<D> &node) {
	return node.output(os, 1);
}

#endif /* SRC_NODE_H_ */
