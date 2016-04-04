/*
 * AHCIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include <stdexcept>
#include <assert.h>
#include "../nodes/Node.h"
#include "../nodes/AHC.h"
#include "../iterators/AHCIterator.h"

AHCIterator::AHCIterator(AHC& node) : NodeIterator() {
	node_ = &node;
	setAddress(0);
}

AHCIterator::AHCIterator(long address, AHC& node) : NodeIterator(address) {
	node_ = &node;
	setAddress(address);
}

AHCIterator::~AHCIterator() {}

void AHCIterator::setAddress(size_t address) {
	if (address >= (1 << node_->dim_)) {
		address_ = (1 << node_->dim_);
	} else {
		// find first filled address if the given one is not filled
		for (address_ = address; !node_->filled_[address_] && address <=(1 << node_->dim_); address_++) {}
		if ((address_ == (1<< node_->dim_) - 1) && !node_->filled_[address_]) address_++;
	}
}

NodeIterator& AHCIterator::operator++() {
	// skip all unfilled fields until the highest address is reached
	for (address_++; !node_->filled_[address_] && address_ < (1 << node_->dim_); address_++) {}
	if ((address_ == (1<< node_->dim_) - 1) && !node_->filled_[address_]) address_++;
	return *this;
}

NodeIterator AHCIterator::operator++(int) {
	 throw std::runtime_error("not implemented");
}

NodeAddressContent AHCIterator::operator*() {

	NodeAddressContent content;
	if (!node_->filled_[address_]) {
		content.exists = false;
	} else {
		content.exists = true;
		content.address = address_;
		content.hasSubnode = node_->hasSubnode_[address_];
		if (content.hasSubnode) {
			content.subnode = node_->subnodes_[address_];
		} else {
			content.suffix = &(node_->suffixes_[address_]);
		}
	}

	return content;
}
