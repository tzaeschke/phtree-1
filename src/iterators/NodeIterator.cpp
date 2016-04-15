/*
 * NodeIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include "iterators/NodeIterator.h"
#include <stdexcept>

NodeIterator::NodeIterator() {
	reachedEnd_ = false;
}

NodeIterator::NodeIterator(long address) {
	reachedEnd_ = false;
}

NodeIterator::~NodeIterator() { }

bool NodeIterator::operator==(const NodeIterator& rhs) {
	 return address_ == rhs.address_;
}

bool NodeIterator::operator!=(const NodeIterator& rhs) {
	 return address_ != rhs.address_;
}

NodeIterator& NodeIterator::operator++() {
	throw std::runtime_error("subclass should implement this");
}

NodeIterator NodeIterator::operator++(int) {
	throw std::runtime_error("subclass should implement this");
}

NodeAddressContent NodeIterator::operator*() {
	throw std::runtime_error("subclass should implement this");
}

void NodeIterator::setAddress(size_t address) {
	throw std::runtime_error("subclass should implement this");
}
