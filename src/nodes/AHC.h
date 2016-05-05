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
#include "nodes/TNode.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "nodes/AHCAddressContent.h"
#include "util/MultiDimBitset.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class AHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class AHC: public TNode<DIM, PREF_BLOCKS> {
	friend class AHCIterator<DIM, PREF_BLOCKS>;
	friend class AssertionVisitor<DIM>;
	friend class SizeVisitor<DIM>;
public:
	AHC();
	AHC(TNode<DIM, PREF_BLOCKS>* node);
	virtual ~AHC();
	NodeIterator<DIM>* begin() override;
	NodeIterator<DIM>* end() override;
	std::ostream& output(std::ostream& os, size_t depth) override;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) override;
	virtual void recursiveDelete() override;
	virtual size_t getNumberOfContents() const override;

protected:
	// TODO use arrays instead by templating the dimensions
	AHCAddressContent<DIM> contents_[1<<DIM];

	void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) override;
	void insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) override;
	void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) override;
	Node<DIM>* adjustSize() override;
};

#include <assert.h>
#include "nodes/AHC.h"
#include "iterators/AHCIterator.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "visitors/Visitor.h"

using namespace std;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::AHC() : TNode<DIM, PREF_BLOCKS>() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::AHC(TNode<DIM, PREF_BLOCKS>* other) : TNode<DIM, PREF_BLOCKS>() {

	// TODO use more efficient way to convert LHC->AHC
	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = other->end();
	for (it = other->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			contents_[content.address] = AHCAddressContent<DIM>(content.subnode);
		} else {
			assert (content.suffixStartBlock);
			contents_[content.address] = AHCAddressContent<DIM>(content.suffixStartBlock, content.id);
		}
	}

	delete it;
	delete endIt;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::~AHC() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::recursiveDelete() {
	for (size_t i = 0; i < 1uL << DIM; ++i) {
		if (contents_[i].filled && contents_[i].hasSubnode) {
			assert (contents_[i].subnode);
			contents_[i].subnode->recursiveDelete();
		}
	}

	delete this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t AHC<DIM, PREF_BLOCKS>::getNumberOfContents() const {
	size_t count = 0;
	for (size_t i = 0; i < 1uL << DIM; ++i) {
		if (contents_[i].filled) {
			++count;
		}
	}

	return count;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent) {
	assert (address < 1uL << DIM);

	outContent.address = address;
	outContent.exists = contents_[address].filled;
	outContent.hasSubnode = contents_[address].hasSubnode;

	if (outContent.exists) {
		if (outContent.hasSubnode) {
			outContent.subnode = contents_[address].subnode;
		} else {
			outContent.suffixStartBlock = contents_[address].suffixStartBlock;
			outContent.id = contents_[address].id;
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, unsigned long* suffixStartBlock, int id) {
	assert (hcAddress < 1ul << DIM);
	contents_[hcAddress] = AHCAddressContent<DIM>(suffixStartBlock, id);
	assert(Node<DIM>::lookup(hcAddress).id == id);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) {
	assert (hcAddress < 1ul << DIM);
	contents_[hcAddress] = AHCAddressContent<DIM>(subnode);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
Node<DIM>* AHC<DIM, PREF_BLOCKS>::adjustSize() {
	// TODO currently there is no need to switch from AHC to LHC because there is no delete function
	return this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::begin() {
	return new AHCIterator<DIM, PREF_BLOCKS>(*this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::end() {
	return new AHCIterator<DIM, PREF_BLOCKS>(1uL << DIM, *this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::accept(Visitor<DIM>* visitor, size_t depth)  {
	visitor->visit(this, depth);
	Node<DIM>::accept(visitor, depth);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
ostream& AHC<DIM, PREF_BLOCKS>::output(ostream& os, size_t depth) {
	os << "AHC";
	Entry<DIM> prefix(this->prefix_, 0);
	os << " | prefix: " << prefix << endl;

	for (size_t address = 0; address < (1uL << DIM); ++address) {
		// print address
		if (contents_[address].filled){
			for (size_t i = 0; i < depth; i++) { os << "-";}
			os << " " << address << ": ";
		}

		// print subnode or prefix
		if (contents_[address].filled && contents_[address].hasSubnode) {
			contents_[address].subnode->output(os, depth + 1);
		} else if (contents_[address].filled) {
			Entry<DIM> suffix(suffixes_[address], 0);
			os << " suffix: " << suffix;
			os << " (id: " << contents_[address].id << ")" << endl;
		}
	}

	return os;
}

#endif /* SRC_AHC_H_ */
