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
	Entry(std::vector<int> values, int bitLength);
	Entry(std::vector<std::vector<bool>> values);
	~Entry();

	// value -> bit
	std::vector<std::vector<bool>> values_;

};

#endif /* SRC_ENTRY_H_ */
