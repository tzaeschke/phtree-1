/*
 * AHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <assert.h>
#include "AHC.h"
#include "../iterators/AHCIterator.h"
#include "../iterators/NodeIterator.h"
#include "NodeAddressContent.h"
#include "../visitors/Visitor.h"

using namespace std;

AHC::AHC(size_t dim, size_t valueLength) :
		Node(dim, valueLength) {
	long maxElements = 1 << dim;
	filled_ = vector<bool>(maxElements, false);
	hasSubnode_ = vector<bool>(maxElements, false);
	subnodes_ = vector<Node*>(maxElements);
	suffixes_ = vector<vector<vector<bool>>>(maxElements);
	prefix_ = vector<vector<bool>>(dim_);
}

AHC::AHC(Node& other) : Node(other) {
	long maxElements = 1 << dim_;
	filled_ = vector<bool>(maxElements, false);
	hasSubnode_ = vector<bool>(maxElements, false);
	subnodes_ = vector<Node*>(maxElements);
	suffixes_ = vector<vector<vector<bool>>>(maxElements);

	NodeIterator* it;
	NodeIterator* endIt = other.end();
	for (it = other.begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent content = *(*it);
		assert (content.exists);
		filled_[content.address] = true;
		if (content.hasSubnode) {
			hasSubnode_[content.address] = true;
			subnodes_[content.address] = content.subnode;
		} else {
			hasSubnode_[content.address] = false;
			suffixes_[content.address] = *content.suffix;
		}
	}

	delete it;
	delete endIt;
}

AHC::~AHC() {
	hasSubnode_.clear();
	filled_.clear();
	for (size_t i = 0; i < subnodes_.size(); i++) {
		if (subnodes_.at(i))
			delete subnodes_.at(i);
	}
	subnodes_.clear();
	suffixes_.clear();
}

void AHC::recursiveDelete() {
	for (size_t i = 0; i < subnodes_.size(); i++) {
		if (subnodes_.at(i))
			subnodes_.at(i)->recursiveDelete();
	}

	delete this;
}

NodeAddressContent AHC::lookup(long address) {

	NodeAddressContent content;
	content.exists = filled_[address];
	content.address = address;
	content.hasSubnode = false;

	if (content.exists) {
		content.hasSubnode = hasSubnode_[address];
		if (content.hasSubnode) {
			content.subnode = subnodes_[address];
		} else {
			content.suffix = &suffixes_[address];
		}
	}

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() == dim_))
					&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

void AHC::insertAtAddress(long hcAddress, vector<vector<bool>>* suffix) {
	filled_[hcAddress] = true;
	hasSubnode_[hcAddress] = false;
	suffixes_[hcAddress] = *suffix;

	assert(lookup(hcAddress).suffix->size() == dim_);
}

void AHC::insertAtAddress(long hcAddress, Node* subnode) {
	filled_[hcAddress] = true;
	hasSubnode_[hcAddress] = true;
	subnodes_[hcAddress] = subnode;
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

void AHC::accept(Visitor* visitor, size_t depth) {
	visitor->visit(this, depth);
	Node::accept(visitor, depth);
}

ostream& AHC::output(ostream& os, size_t depth) {
	os << "AHC";
	Entry prefix(prefix_);
	os << " | prefix: " << prefix << endl;

	for (long address = 0; address < (2L << dim_); address++) {
		// print address
		if (filled_[address]){
			for (size_t i = 0; i < depth; i++) { os << "-";}
			os << " " << address << ": ";
		}

		// print subnode or prefix
		if (hasSubnode_[address]) {
			subnodes_[address]->output(os, depth + 1);
		} else if (filled_[address]) {
			Entry suffix(suffixes_[address]);
			os << " suffix: " << suffix << endl;
		}
	}

	return os;
}

