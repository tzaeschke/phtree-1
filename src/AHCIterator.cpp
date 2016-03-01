/*
 * AHCIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include <assert.h>
#include "Node.h"
#include "AHC.h"
#include "AHCIterator.h"

AHCIterator::AHCIterator(AHC& node) : NodeIterator() {
	node_ = &node;
}

AHCIterator::AHCIterator(long address, AHC& node) : NodeIterator(address) {
	node_ = &node;
}

AHCIterator::~AHCIterator() {}

NodeIterator& AHCIterator::operator++() {
	// skip all unfilled fields
	for (address_++; !node_->filled_[address_];	address_++) {}
	return *this;
}

NodeIterator AHCIterator::operator++(int) {
	 throw "not implemented";
}

bool AHCIterator::operator==(const NodeIterator& rhs) {
	 throw "== not implemented";
}

bool AHCIterator::operator!=(const NodeIterator& rhs) {
	 throw "!= not implemented";
}

NodeAddressContent& AHCIterator::operator*() {

	NodeAddressContent* content = new NodeAddressContent;
	content->contained = node_->filled_[address_];
	assert (content->contained);
	content->hasSubnode = node_->hasSubnode_[address_];
	if (content->hasSubnode) {
		content->subnode = node_->subnodes_[address_];
	} else {
		content->suffix = &(node_->suffixes_[address_]);
	}

	return *content;
}
