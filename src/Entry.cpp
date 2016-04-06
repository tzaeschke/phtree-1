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

Entry::Entry(vector<long> values, int bitLength, int id) : id_(id) {
	values_.resize(values.size());
	MultiDimBitTool::longsToBitsets(values_, values, bitLength);
}

Entry::Entry(vector<vector<bool>> values, int id) : values_(values), id_(id) {
}

Entry::~Entry() {
	values_.clear();
}

size_t Entry::getBitLength() {
	if (values_.empty()) {
		return 0;
	} else {
		return values_[0].size();
	}
}

size_t Entry::getDimensions() {
	return values_.size();
}

ostream& operator <<(ostream& os, const Entry &e) {
	os << "(";
	for (size_t value = 0; value < e.values_.size(); value++) {
		assert (e.values_[value].size() == e.values_[0].size());
		os << "(";
		for (size_t bit = 0; bit < e.values_[value].size(); bit++) {
			int bitNumber = (e.values_[value][bit]) ? 1 : 0;
			os << bitNumber;
		}

		os << ")";
		if (value != e.values_.size() -1)
			os << ", ";
	}
	os << ")";

	return os;
}

bool operator ==(const Entry &entry1, const Entry &entry2) {
	if (entry1.values_.size() != entry2.values_.size()) {
		return false;
	}

	for (size_t i = 0; i < entry1.values_.size(); i++) {
		if (entry1.values_.at(i).size() != entry2.values_.at(i).size()) {
			return false;
		}
		for (size_t j = 0; j < entry1.values_.at(i).size(); j++) {
			if (entry1.values_.at(i).at(j) != entry2.values_.at(i).at(j)) {
				return false;
			}
		}
	}

	return true;
}

bool operator !=(const Entry &entry1, const Entry &entry2) {
	return !(entry1 == entry2);
}
