/*
 * Entry.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include "util/MultiDimBitTool.h"

using namespace std;

#include "Entry.h"

Entry::Entry(vector<long> values, int bitLength, int id) : id_(id), dim_(values.size()) {
	MultiDimBitTool::longsToBitsets(values_, values, bitLength, dim_);
	assert (values_.size() == getBitLength() * getDimensions());
}

Entry::Entry(vector<bool> values, size_t dim, int id) : values_(values), id_(id), dim_(dim) {
	assert (values_.size() == getBitLength() * getDimensions());
}

Entry::~Entry() {
	values_.clear();
}

size_t Entry::getBitLength() {
	return values_.size() / dim_;
}

size_t Entry::getDimensions() {
	return dim_;
}

ostream& operator <<(ostream& os, const Entry &e) {
	os << "(";
	const size_t dim = e.dim_;
	const size_t bitLength = e.values_.size() / dim;
	for (size_t d = 0; d < dim; ++d) {
		os << "(";
		for (size_t i = 0; i < bitLength; ++i) {
			const int bitNumber = (e.values_[i * dim + d]) ? 1 : 0;
			os << bitNumber;
		}
		os << ")";
		if (d < dim - 1) {
			os << ", ";
		}
	}

	os << ")";
	return os;
}

bool operator ==(const Entry &entry1, const Entry &entry2) {
	if (entry1.values_.size() != entry2.values_.size()) {
		return false;
	}

	for (size_t i = 0; i < entry1.values_.size(); i++) {
		if (entry1.values_[i] != entry2.values_[i]) {
			return false;
		}
	}

	return true;
}

bool operator !=(const Entry &entry1, const Entry &entry2) {
	return !(entry1 == entry2);
}
