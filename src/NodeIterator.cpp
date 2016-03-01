/*
 * NodeIterator.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include "NodeIterator.h"

NodeIterator::NodeIterator() {
	address_ = 0;
}

NodeIterator::NodeIterator(long address) {
	address_ = address;
}

NodeIterator::~NodeIterator() { }

NodeIterator& NodeIterator::operator++() {
	 throw "not implemented";
}

NodeIterator NodeIterator::operator++(int) {
	 throw "not implemented";
}

bool NodeIterator::operator==(const NodeIterator& rhs) {
	 throw "not implemented";
}

bool NodeIterator::operator!=(const NodeIterator& rhs) {
	 throw "not implemented";
}

NodeAddressContent& NodeIterator::operator*() {
	 throw "not implemented";
}
