/*
 * PHTree.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "PHTree.h"
#include "nodes/LHC.h"
#include <assert.h>

using namespace std;
#define DEBUG false

PHTree::PHTree(int dim, int valueLength) {
	valueLength_ = valueLength;
	dim_ = dim;
	root_ = new LHC(dim, valueLength);
}

PHTree::~PHTree() {
	root_->recursiveDelete();
}

void PHTree::insert(Entry* e) {
	assert (e->getBitLength() == valueLength_ && "value length of new entries must match the tree's");
	assert (e->getDimensions() == dim_ && "entry dimension must match the tree dimension");

	if (DEBUG)
		cout << "inserting: " << *e << endl;
	Node* updatedRoot = root_->insert(e, 0, 0);
	if (updatedRoot != root_) {
		delete root_;
		root_ = updatedRoot;
	}
}

std::pair<bool,int> PHTree::lookup(Entry* e) {
	assert (e->getBitLength() == valueLength_ && "value length of entries must match the tree's");
	assert (e->getDimensions() == dim_ && "entry dimension must match the tree dimension");

	if (DEBUG)
		cout << "searching: " << *e << endl;
	return root_->lookup(e, 0, 0, NULL);
}

RangeQueryIterator* PHTree::rangeQuery(Entry* lowerLeft, Entry* upperRight) {
	assert (lowerLeft->getBitLength() == valueLength_ && "value length of the lower left corner must match the tree's");
	assert (upperRight->getBitLength() == valueLength_ && "value length of the upper right corner must match the tree's");
	assert (lowerLeft->getDimensions() == dim_ && upperRight->getDimensions() == dim_ && "entry dimension must match the tree dimension");
	// TODO check of lower left and upper right corners are correctly set

	return root_->rangeQuery(lowerLeft, upperRight, 0, 0);
}

void PHTree::accept(Visitor* visitor) {
	root_->accept(visitor, 0);
}

ostream& operator <<(ostream& os, const PHTree &tree) {
	os << "PH-Tree (dim=" << tree.dim_ << ", value length=" << tree.valueLength_ << ")" << endl;
	os << *tree.root_;
	return os;
}
