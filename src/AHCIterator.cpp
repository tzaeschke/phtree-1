/*
 * AHCIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include <stdexcept>
#include <assert.h>
#include "Node.h"
#include "AHC.h"
#include "AHCIterator.h"

AHCIterator::AHCIterator(AHC& node) : NodeIterator() {
	node_ = &node;

	// find first valid address
	for (; !node_->filled_[address_]; address_++) {}
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
	 throw std::runtime_error("not implemented");
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
