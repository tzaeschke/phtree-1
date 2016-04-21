/*
 * AHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <assert.h>
#include "nodes/AHC.h"
#include "iterators/AHCIterator.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "visitors/Visitor.h"

using namespace std;

AHC::AHC(size_t dim, size_t valueLength) : Node(dim, valueLength),
		contents_(1uL <<dim), suffixes_(1uL <<dim_) {
}

AHC::AHC(Node& other) : Node(other),
		contents_(1uL <<dim_), suffixes_(1uL <<dim_) {

	// TODO use more efficient way to convert LHC->AHC
	NodeIterator* it;
	NodeIterator* endIt = other.end();
	for (it = other.begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			contents_[content.address] = AHCAddressContent(content.subnode);
		} else {
			assert (content.suffix);
			contents_[content.address] = AHCAddressContent(content.id);
			suffixes_[content.address] = *content.suffix;
		}
	}

	delete it;
	delete endIt;
}

AHC::~AHC() {
	contents_.clear();
	suffixes_.clear();
}

void AHC::recursiveDelete() {
	for (size_t i = 0; i < 1uL <<dim_; ++i) {
		if (contents_[i].filled && contents_[i].hasSubnode) {
			assert (contents_[i].subnode);
			contents_[i].subnode->recursiveDelete();
		}
	}

	delete this;
}

NodeAddressContent AHC::lookup(unsigned long address) {
	assert (address < 1uL <<dim_);

	NodeAddressContent content;
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

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() % dim_ == 0))
					&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

MultiDimBitset* AHC::insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) {
	assert (hcAddress < 1ul <<dim_);

	contents_[hcAddress] = AHCAddressContent(id);
	suffixes_[hcAddress].setDim(dim_);

	assert(lookup(hcAddress).id == id);
	return &(suffixes_[hcAddress]);
}

void AHC::insertAtAddress(unsigned long hcAddress, Node* subnode) {
	assert (hcAddress < 1ul <<dim_);
	contents_[hcAddress] = AHCAddressContent(subnode);
}

Node* AHC::adjustSize() {
	// TODO currently there is no need to switch from AHC to LHC because there is no delete function
	return this;
}

NodeIterator* AHC::begin() {
	return new AHCIterator(*this);
}

NodeIterator* AHC::end() {
	return new AHCIterator(1<<dim_, *this);
}

void AHC::accept(Visitor* visitor, size_t depth)  {
	visitor->visit(this, depth);
	Node::accept(visitor, depth);
}

ostream& AHC::output(ostream& os, size_t depth) {
	os << "AHC";
	Entry prefix(prefix_, dim_, 0);
	os << " | prefix: " << prefix << endl;

	for (size_t address = 0; address < (1uL << dim_); ++address) {
		// print address
		if (contents_[address].filled){
			for (size_t i = 0; i < depth; i++) { os << "-";}
			os << " " << address << ": ";
		}

		// print subnode or prefix
		if (contents_[address].filled && contents_[address].hasSubnode) {
			contents_[address].subnode->output(os, depth + 1);
		} else if (contents_[address].filled) {
			Entry suffix(suffixes_[address], dim_, 0);
			os << " suffix: " << suffix;
			os << " (id: " << contents_[address].id << ")" << endl;
		}
	}

	return os;
}

