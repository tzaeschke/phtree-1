/*
 * Entry.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_ENTRY_H_
#define SRC_ENTRY_H_

#include <iostream>
#include "Entry.h"

class Entry {
	friend std::ostream& operator <<(std::ostream &out, const Entry &entry);
	friend bool operator ==(const Entry &entry1, const Entry &entry2);
	friend bool operator !=(const Entry &entry1, const Entry &entry2);

public:

	Entry(std::vector<long> values, int bitLength, int id);
	Entry(std::vector<bool> values, size_t dim, int id);
	~Entry();

	size_t getBitLength();
	size_t getDimensions();

	// value -> bit
	size_t dim_;
	std::vector<bool> values_;
	int id_;

};

#endif /* SRC_ENTRY_H_ */
