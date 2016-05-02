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

template <unsigned int DIM>
class AHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM>
class AHC: public Node<DIM> {
	friend class AHCIterator<DIM>;
	friend class AssertionVisitor<DIM>;
	friend class SizeVisitor<DIM>;
public:
	AHC(size_t dim, size_t valueLength);
	AHC(Node<DIM>& node);
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
	MultiDimBitset<DIM> suffixes_[1<<DIM];

	virtual NodeAddressContent<DIM> lookup(unsigned long address) override;
	virtual MultiDimBitset<DIM>* insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) override;
	virtual void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) override;
	virtual Node<DIM>* adjustSize() override;
};

#include <assert.h>
#include "nodes/AHC.h"
#include "iterators/AHCIterator.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "visitors/Visitor.h"

using namespace std;

template <unsigned int DIM>
AHC<DIM>::AHC(size_t dim, size_t valueLength) : Node<DIM>(dim, valueLength) {
}

template <unsigned int DIM>
AHC<DIM>::AHC(Node<DIM>& other) : Node<DIM>(other) {

	// TODO use more efficient way to convert LHC->AHC
	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = other.end();
	for (it = other.begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			contents_[content.address] = AHCAddressContent<DIM>(content.subnode);
		} else {
			assert (content.suffix);
			contents_[content.address] = AHCAddressContent<DIM>(content.id);
			suffixes_[content.address] = *content.suffix;
		}
	}

	delete it;
	delete endIt;
}

template <unsigned int DIM>
AHC<DIM>::~AHC() {
}

template <unsigned int DIM>
void AHC<DIM>::recursiveDelete() {
	for (size_t i = 0; i < 1uL << DIM; ++i) {
		if (contents_[i].filled && contents_[i].hasSubnode) {
			assert (contents_[i].subnode);
			contents_[i].subnode->recursiveDelete();
		}
	}

	delete this;
}

template <unsigned int DIM>
size_t AHC<DIM>::getNumberOfContents() const {
	size_t count = 0;
	for (size_t i = 0; i < 1uL << DIM; ++i) {
		if (contents_[i].filled) {
			++count;
		}
	}

	return count;
}

template <unsigned int DIM>
NodeAddressContent<DIM> AHC<DIM>::lookup(unsigned long address) {
	assert (address < 1uL << DIM);

	NodeAddressContent<DIM> content;
	content.address = address;
	content.exists = contents_[address].filled;
	content.hasSubnode = contents_[address].hasSubnode;

	if (content.exists) {
		if (content.hasSubnode) {
			content.subnode = contents_[address].subnode;
		} else {
			content.suffix = &suffixes_[address];
			content.id = contents_[address].id;
		}
	}

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() % DIM == 0))
					&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

template <unsigned int DIM>
MultiDimBitset<DIM>* AHC<DIM>::insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) {
	assert (hcAddress < 1ul << DIM);

	contents_[hcAddress] = AHCAddressContent<DIM>(id);

	assert(lookup(hcAddress).id == id);
	return &(suffixes_[hcAddress]);
}

template <unsigned int DIM>
void AHC<DIM>::insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) {
	assert (hcAddress < 1ul << DIM);
	contents_[hcAddress] = AHCAddressContent<DIM>(subnode);
}

template <unsigned int DIM>
Node<DIM>* AHC<DIM>::adjustSize() {
	// TODO currently there is no need to switch from AHC to LHC because there is no delete function
	return this;
}

template <unsigned int DIM>
NodeIterator<DIM>* AHC<DIM>::begin() {
	return new AHCIterator<DIM>(*this);
}

template <unsigned int DIM>
NodeIterator<DIM>* AHC<DIM>::end() {
	return new AHCIterator<DIM>(1uL << DIM, *this);
}

template <unsigned int DIM>
void AHC<DIM>::accept(Visitor<DIM>* visitor, size_t depth)  {
	visitor->visit(this, depth);
	Node<DIM>::accept(visitor, depth);
}

template <unsigned int DIM>
ostream& AHC<DIM>::output(ostream& os, size_t depth) {
	os << "AHC";
	Entry<DIM> prefix(this->prefix_, DIM, 0);
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
			Entry<DIM> suffix(suffixes_[address], DIM, 0);
			os << " suffix: " << suffix;
			os << " (id: " << contents_[address].id << ")" << endl;
		}
	}

	return os;
}

#endif /* SRC_AHC_H_ */
