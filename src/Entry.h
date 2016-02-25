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

using namespace std;

class Entry {
public:
	Entry(vector<int> values, int bitLength);
	~Entry();

	// value -> bit
	vector<vector<bool>> values_;

	friend ostream& operator <<(ostream &out, const Entry &entry);
};

#endif /* SRC_ENTRY_H_ */
