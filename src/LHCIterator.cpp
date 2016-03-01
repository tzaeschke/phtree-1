/*
 * LHCIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include "LHCIterator.h"

LHCIterator::LHCIterator(LHC& node) : NodeIterator() {
	node_ = &node;
	contentMapIt_ = node_->sortedContents_->begin();
}

LHCIterator::LHCIterator(long address, LHC& node) : NodeIterator(address) {
	node_ = &node;
	contentMapIt_ = node_->sortedContents_->begin();
}

LHCIterator::~LHCIterator() { }

NodeIterator& LHCIterator::operator++() {
	contentMapIt_++;
	return *this;
}

NodeIterator LHCIterator::operator++(int i) {
	throw "++ i not implemented";
}

bool LHCIterator::operator==(const NodeIterator& rhs) {
	 throw "== not implemented";
}

bool LHCIterator::operator!=(const NodeIterator& rhs) {
	 throw "!= not implemented";
}

Node::NodeAddressContent& LHCIterator::operator*() {
	return *(contentMapIt_->second);
}
