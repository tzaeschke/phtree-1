/*
 * Entry.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_ENTRY_H_
#define SRC_ENTRY_H_

#include <vector>
#include <iostream>
#include "util/MultiDimBitset.h"

template <unsigned int DIM>
class Entry {
	template <unsigned int D>
	friend std::ostream& operator <<(std::ostream &out, const Entry<D> &entry);
	template <unsigned int D>
	friend bool operator ==(const Entry<D> &entry1, const Entry<D> &entry2);
	template <unsigned int D>
	friend bool operator !=(const Entry<D> &entry1, const Entry<D> &entry2);

public:

	Entry(std::vector<unsigned long> &values, size_t bitLength, int id);
	Entry(MultiDimBitset<DIM> values, size_t dim, int id);
	~Entry();

	size_t getBitLength() const;
	size_t getDimensions() const;

	// value -> bit
	size_t dim_;
	int id_;
	MultiDimBitset<DIM> values_;
};

#include <string>
#include <assert.h>

using namespace std;

template <unsigned int DIM>
Entry<DIM>::Entry(vector<unsigned long> &values, size_t bitLength, int id) : dim_(values.size()), id_(id) {
	values_ = MultiDimBitset<DIM>(values, bitLength);
	assert (values_.size() == getBitLength() * getDimensions());
}

template <unsigned int DIM>
Entry<DIM>::Entry(MultiDimBitset<DIM> values, size_t dim, int id) : dim_(dim), id_(id), values_(values) {
	assert (values_.size() == getBitLength() * getDimensions());
}

template <unsigned int DIM>
Entry<DIM>::~Entry() {
	values_.clear();
}

template <unsigned int DIM>
size_t Entry<DIM>::getBitLength() const {
	return values_.size() / dim_;
}

template <unsigned int DIM>
size_t Entry<DIM>::getDimensions() const {
	return dim_;
}

template <unsigned int D>
ostream& operator <<(ostream& os, const Entry<D> &e) {
	os << e.values_;
	return os;
}

template <unsigned int D>
bool operator ==(const Entry<D> &entry1, const Entry<D> &entry2) {
	return entry1.values_ == entry2.values_;
}

template <unsigned int D>
bool operator !=(const Entry<D> &entry1, const Entry<D> &entry2) {
	return !(entry1 == entry2);
}

#endif /* SRC_ENTRY_H_ */
