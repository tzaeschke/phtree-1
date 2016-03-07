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
	contentMapIt_ = node_->sortedContents_->begin();
	address_ = contentMapIt_->first;
}

LHCIterator::LHCIterator(long address, LHC& node) : NodeIterator(address) {
	node_ = &node;
	contentMapIt_ = node_->sortedContents_->begin();
}

LHCIterator::~LHCIterator() { }

NodeIterator& LHCIterator::operator++() {

	if (++contentMapIt_ != node_->sortedContents_->end()) {
		address_ = contentMapIt_->first;
	} else {
		--contentMapIt_;
		reachedEnd_ = true;
	}
	return *this;
}

NodeIterator LHCIterator::operator++(int i) {
	throw "++ i not implemented";
}

NodeAddressContent& LHCIterator::operator*() {
	assert (contentMapIt_->second->address == address_);
	return *(contentMapIt_->second);
}
