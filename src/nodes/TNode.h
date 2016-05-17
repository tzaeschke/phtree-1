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
class SizeVisitor;
template <unsigned int DIM>
class PrefixSharingVisitor;
template <unsigned int DIM, unsigned int WIDTH>
class DynamicNodeOperationsUtil;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class TNode : public Node<DIM> {
	// TODO remove friends and use getters and setters
	friend class NodeIterator<DIM>;
	friend class SizeVisitor<DIM>;
	friend class PrefixSharingVisitor<DIM>;
public:

	TNode(size_t prefixLength);
	TNode(TNode<DIM, PREF_BLOCKS>* other);
	virtual ~TNode() {}
	virtual std::ostream& output(std::ostream& os, size_t depth, size_t index, size_t totalBitLength) override;
	virtual NodeIterator<DIM>* begin() const = 0;
	virtual NodeIterator<DIM>* end() const = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) override;
	virtual void recursiveDelete() = 0;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;
	virtual size_t getMaximumNumberOfContents() const = 0;
	size_t getPrefixLength() const override;
	unsigned long* getPrefixStartBlock() override;
	const unsigned long* getFixPrefixStartBlock() const override;
	virtual void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) const = 0;
	NodeAddressContent<DIM> lookup(unsigned long address) const override;
	virtual void insertAtAddress(unsigned long hcAddress, const unsigned long* const startSuffixBlock, int id) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) = 0;
	virtual Node<DIM>* adjustSize() = 0;

protected:
	size_t prefixBits_;
	// TODO template block type
	unsigned long prefix_[PREF_BLOCKS];
	virtual string getName() const =0;
};

#include <assert.h>
#include <stdexcept>
#include "nodes/LHC.h"
#include "util/SpatialSelectionOperationsUtil.h"
#include "iterators/RangeQueryIterator.h"
#include "iterators/NodeIterator.h"

using namespace std;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
TNode<DIM, PREF_BLOCKS>::TNode(size_t prefixLength) : prefixBits_(prefixLength * DIM), prefix_() {
	assert (prefixBits_ <= PREF_BLOCKS * sizeof (unsigned long) * 8);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
TNode<DIM, PREF_BLOCKS>::TNode(TNode<DIM, PREF_BLOCKS>* other) : prefixBits_(other->prefixBits_) {
	assert (prefixBits_ <= PREF_BLOCKS * sizeof (unsigned long) * 8);
	for (unsigned i = 0; i < PREF_BLOCKS; ++i) {
		prefix_[i] = other->prefix_[i];
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t TNode<DIM, PREF_BLOCKS>::getPrefixLength() const {
	return prefixBits_ / DIM;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
unsigned long* TNode<DIM, PREF_BLOCKS>::getPrefixStartBlock() {
	return prefix_;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
const unsigned long* TNode<DIM, PREF_BLOCKS>::getFixPrefixStartBlock() const {
	return prefix_;
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
NodeAddressContent<DIM> TNode<DIM, PREF_BLOCKS>::lookup(unsigned long address) const {
	NodeAddressContent<DIM> content;
	this->lookup(address, content);
	return content;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
ostream& TNode<DIM, PREF_BLOCKS>::output(std::ostream& os, size_t depth, size_t index, size_t totalBitLength) {
	os << this->getName() << " | prefix: ";
	MultiDimBitset<DIM>::output(os, prefix_, prefixBits_);
	os << endl;
	const size_t currentIndex = index + this->getPrefixLength() + 1;

	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = this->end();
	for (it = this->begin(); *it != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = (*(*it));
		for (size_t i = 0; i < depth; i++) {os << "-";}
		os << " " << content.address << ": ";

		if (content.hasSubnode) {
			// print subnode
			content.subnode->output(os, depth + 1, currentIndex, totalBitLength);
		} else {
			// print suffix
			os << " suffix: ";
			MultiDimBitset<DIM>::output(os, content.suffixStartBlock, DIM * (totalBitLength - currentIndex));
			os << " (id: " << content.id << ")" << endl;
		}
	}

	delete it;
	delete endIt;
	return os;
}

#endif /* SRC_NODES_TNODE_H_ */
