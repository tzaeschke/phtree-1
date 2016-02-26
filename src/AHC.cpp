/*
 * AHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "AHC.h"

AHC::AHC(size_t dim, size_t valueLength) :
		Node(dim, valueLength) {
	valueLength_ = valueLength;
	dim_ = dim;
	long maxElements = 1 << dim;
	filled_ = vector<bool>(maxElements, false);
	hasSubnode_ = vector<bool>(maxElements, false);
	subnodes_ = vector<Node*>(maxElements);
	suffixes_ = vector<vector<vector<bool>>>(maxElements);
	prefix_ = vector<vector<bool>>(dim_);
}

AHC::~AHC() {
	delete &hasSubnode_;
	delete &filled_;
	for (size_t i = 0; i < subnodes_.size(); i++) {
		delete subnodes_[i];
	}
	delete &subnodes_;
	delete &suffixes_;
}

Node::NodeAddressContent AHC::lookup(long address) {
	NodeAddressContent content;
	content.contained = filled_[address];
	content.hasSubnode = hasSubnode_[address];
	if (content.hasSubnode) {
		content.subnode = subnodes_[address];
	} else {
		content.suffix = &suffixes_[address];
	}
	return content;
}

void AHC::insertAtAddress(long hcAddress, vector<vector<bool>>* suffix) {
	filled_[hcAddress] = true;
	hasSubnode_[hcAddress] = false;
	suffixes_[hcAddress] = *suffix;
}

void AHC::insertAtAddress(long hcAddress, Node* subnode) {
	filled_[hcAddress] = true;
	hasSubnode_[hcAddress] = true;
	subnodes_[hcAddress] = subnode;
}

ostream& AHC::output(ostream& os, size_t depth) {
	os << "AHC" << endl;
	for (long address = 0; address < (2L << dim_); address++) {
		if (hasSubnode_[address]) {
			for (size_t i = 0; i < depth; i++) { os << "-";}
			subnodes_[address]->output(os, depth + 1);
		}
	}

	return os;
}

