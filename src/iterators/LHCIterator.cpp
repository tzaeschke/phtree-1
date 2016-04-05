/*
 * LHCIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include <assert.h>
#include "LHCIterator.h"

LHCIterator::LHCIterator(LHC& node) : NodeIterator() {
	node_ = &node;
	setAddress(0);
}

LHCIterator::LHCIterator(long address, LHC& node) : NodeIterator(address) {
	node_ = &node;
	setAddress(address);
}

LHCIterator::~LHCIterator() { }

void LHCIterator::setAddress(size_t address) {
	// find first filled address if the given one is not filled
	// TODO implement without starting at the front
	if (address >= node_->dim_) {
		address_ = 1 << node_->dim_;
	} else {
		contentMapIt_ = node_->sortedContents_.begin();
		for (address_ = contentMapIt_->first; address_ < address && contentMapIt_ != node_->sortedContents_.end(); ++contentMapIt_) {
			address_ = contentMapIt_->first;
		}
	}
}

NodeIterator& LHCIterator::operator++() {

	if (++contentMapIt_ != node_->sortedContents_.end()) {
		address_ = contentMapIt_->first;
	} else {
		--contentMapIt_;
		reachedEnd_ = true;
		address_ = 1 << node_->dim_;
	}
	return *this;
}

NodeIterator LHCIterator::operator++(int i) {
	throw "++ i not implemented";
}

NodeAddressContent LHCIterator::operator*() {
	assert (contentMapIt_->first == address_);
	NodeAddressContent content;
	content.exists = true;
	content.address = contentMapIt_->first;
	content.hasSubnode = contentMapIt_->second.hasSubnode;
	content.subnode = contentMapIt_->second.subnode;
	content.suffix = NULL;
	if (!contentMapIt_->second.suffix.empty()) {
		content.suffix = &(contentMapIt_->second.suffix);
	}

	return content;
}
