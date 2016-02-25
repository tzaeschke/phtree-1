/*
 * Node.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include <vector>
#include "Entry.h"

using namespace std;

class Node {
public:
	// value -> bit
	vector<vector<bool>>  prefix_;

	virtual ~Node();

	virtual void insert(Entry* e, int depth, int index);

	virtual bool lookup(Entry* e, int depth, int index);

	virtual size_t getSuffixSize(long hcAddress);

	size_t getPrefixLength();

	long interleaveBits(int index, Entry* e);

	long interleaveBits(int index, vector<vector<bool> >* values);
};

#endif /* SRC_NODE_H_ */
