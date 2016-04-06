/*
 * PHTree.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_PHTREE_H_
#define SRC_PHTREE_H_

#include <iostream>

class Entry;
class Node;
class Visitor;
class RangeQueryIterator;

class PHTree {
	friend std::ostream& operator<<(std::ostream& os, const PHTree& tree);
public:
	PHTree(int dim, int valueLength);
	virtual ~PHTree();
	void insert(Entry* e);
	std::pair<bool,int> lookup(Entry* e);
	RangeQueryIterator* rangeQuery(Entry* lowerLeft, Entry* upperRight);

	void accept(Visitor* visitor);

protected:
	Node* root_;
	size_t dim_; // dimensions (k)
	size_t valueLength_;
};

#endif /* SRC_PHTREE_H_ */
