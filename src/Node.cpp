/*
 * Node.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "Node.h"

Node::~Node() {
	delete &prefix_;
}

void Node::insert(Entry* e, int depth, int index) {
}

bool Node::lookup(Entry* e, int depth, int index) {
	return false;
}

size_t Node::getSuffixSize(long hcAddress) {
	return 0;
}

size_t Node::getPrefixLength() {
	if (prefix_.size() == 0)
		return 0;
	else
		return prefix_[0].size();
}

long Node::interleaveBits(int index, Entry* e) {
	return interleaveBits(index, &(e->values_));
}

long Node::interleaveBits(int index, vector<vector<bool> >* values) {
	long hcAddress = 0;
	int max = values->size() - 1;
	for (size_t value = 0; value < values->size(); value++) {
		hcAddress |= (*values)[value][index] << (max - value);
	}
	return hcAddress;
}

