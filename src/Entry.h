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

class Entry {
	friend std::ostream& operator <<(std::ostream &out, const Entry &entry);
	friend bool operator ==(const Entry &entry1, const Entry &entry2);
	friend bool operator !=(const Entry &entry1, const Entry &entry2);

public:

	Entry(std::vector<unsigned long> &values, size_t bitLength, int id);
	Entry(MultiDimBitset values, size_t dim, int id);
	~Entry();

	size_t getBitLength() const;
	size_t getDimensions() const;

	// value -> bit
	size_t dim_;
	int id_;
	MultiDimBitset values_;
};

#endif /* SRC_ENTRY_H_ */
