/*
 * TNode.h
 *
 *  Created on: May 4, 2016
 *      Author: max
 */

#ifndef SRC_NODES_TNODE_H_
#define SRC_NODES_TNODE_H_

#include "Entry.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "util/MultiDimBitset.h"
#include "nodes/Node.h"

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

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class TNode : public Node<DIM> {
	// TODO remove friends and use getters and setters
	friend class NodeIterator<DIM>;
	friend class RangeQueryIterator<DIM>;
	friend class SizeVisitor<DIM>;
	friend class PrefixSharingVisitor<DIM>;
public:

	TNode();
	virtual ~TNode() {}
	RangeQueryIterator<DIM>* rangeQuery(const Entry<DIM>* lowerLeft, const Entry<DIM>* upperRight, size_t depth, size_t index) override;

	virtual std::ostream& output(std::ostream& os, size_t depth) =0;
	virtual NodeIterator<DIM>* begin() = 0;
	virtual NodeIterator<DIM>* end() = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) override;
	virtual void recursiveDelete() = 0;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;

protected:
	size_t prefixBits_;
	// TODO template block type
	unsigned long prefix_[PREF_BLOCKS];

	size_t getSuffixSize(NodeAddressContent<DIM>) const override;
	size_t getPrefixLength() const override;

	virtual void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) = 0;
	NodeAddressContent<DIM> lookup(unsigned long address) override;
	virtual void insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) = 0;
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

template <unsigned int DIM, unsigned int PREF_BLOCKS>
TNode<DIM, PREF_BLOCKS>::TNode() : prefixBits_(0) {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
RangeQueryIterator<DIM>* TNode<DIM, PREF_BLOCKS>::rangeQuery(const Entry<DIM>* lowerLeft, const Entry<DIM>* upperRight, size_t depth, size_t index) {
	/*vector<Node<DIM>*>* visitedNodes = new vector<Node<DIM>*>();
	SpatialSelectionOperationsUtil::lookup(lowerLeft, this, visitedNodes);
	RangeQueryIterator<DIM>* iterator = new RangeQueryIterator<DIM>(visitedNodes, DIM, valueLength_, lowerLeft, upperRight);
	return iterator;*/
	return NULL;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t TNode<DIM, PREF_BLOCKS>::getSuffixSize(NodeAddressContent<DIM> content) const {
	if (content.hasSubnode) {
		return 0;
	} else {
		return content.suffix->size() / DIM;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t TNode<DIM, PREF_BLOCKS>::getPrefixLength() const {
	return prefixBits_ / DIM;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void TNode<DIM, PREF_BLOCKS>::accept(Visitor<DIM>* visitor, size_t depth) {
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

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeAddressContent<DIM> TNode<DIM, PREF_BLOCKS>::lookup(unsigned long address) {
	NodeAddressContent<DIM> content;
	this->lookup(address, content);
	return content;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
ostream& TNode<DIM, PREF_BLOCKS>::output(ostream& os, size_t depth) {
	return os << "subclass should overwrite this";
}

#endif /* SRC_NODES_TNODE_H_ */
