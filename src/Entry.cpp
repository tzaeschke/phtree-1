/*
 * Entry.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <iostream>
#include <string>
#include <assert.h>
#include "Entry.h"

using namespace std;


Entry::Entry(vector<unsigned long> &values, size_t bitLength, int id) : dim_(values.size()), id_(id) {
	values_ = MultiDimBitset(values, bitLength, values.size());
	assert (values_.size() == getBitLength() * getDimensions());
}

Entry::Entry(MultiDimBitset values, size_t dim, int id) : dim_(dim), id_(id), values_(values) {
	assert (values_.size() == getBitLength() * getDimensions());
}

Entry::~Entry() {
	values_.clear();
}

size_t Entry::getBitLength() const {
	return values_.size() / dim_;
}

size_t Entry::getDimensions() const {
	return dim_;
}

ostream& operator <<(ostream& os, const Entry &e) {
	os << e.values_;
	return os;
}

bool operator ==(const Entry &entry1, const Entry &entry2) {
	return entry1.values_ == entry2.values_;
}

bool operator !=(const Entry &entry1, const Entry &entry2) {
	return !(entry1 == entry2);
}
