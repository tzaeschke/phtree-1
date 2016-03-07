/*
 * NodeIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include "NodeIterator.h"
#include <stdexcept>

NodeIterator::NodeIterator() {
	address_ = 0;
	reachedEnd_ = false;
}

NodeIterator::NodeIterator(long address) {
	address_ = address;
	reachedEnd_ = false;
}

NodeIterator::~NodeIterator() { }

bool NodeIterator::operator==(const NodeIterator& rhs) {
	 return address_ == rhs.address_;
}

bool NodeIterator::operator!=(const NodeIterator& rhs) {
	 return address_ != rhs.address_;
}

bool NodeIterator::operator<=(const NodeIterator& rhs) {
	 return address_ <= rhs.address_ && !reachedEnd_;
}

NodeIterator& NodeIterator::operator++() {
	throw std::runtime_error("subclass should implement this");
}

NodeIterator NodeIterator::operator++(int) {
	throw std::runtime_error("subclass should implement this");
}

NodeAddressContent& NodeIterator::operator*() {
	throw std::runtime_error("subclass should implement this");
}