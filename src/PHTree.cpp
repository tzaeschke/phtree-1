/*
 * PHTree.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "PHTree.h"
#include "LHC.h"
#include <assert.h>

using namespace std;
#define DEBUG false

PHTree::PHTree(int dim, int valueLength) {
	valueLength_ = valueLength;
	dim_ = dim;
	root_ = new LHC(dim, valueLength);
}

PHTree::~PHTree() {
	delete root_;
}

void PHTree::insert(Entry* e) {
	assert (e->getBitLength() == valueLength_ && "value length of new entries must match the tree's");
	assert (e->getDimensions() == dim_ && "entry dimension must match the tree dimension");

	if (DEBUG)
		cout << "inserting: " << *e << endl;
	root_ = root_->insert(e, 0, 0);
}

bool PHTree::lookup(Entry* e) {
	assert (e->getBitLength() == valueLength_ && "value length of entries must match the tree's");
	assert (e->getDimensions() == dim_ && "entry dimension must match the tree dimension");

	if (DEBUG)
		cout << "searching: " << *e << endl;
	return root_->lookup(e, 0, 0);
}

void PHTree::accept(Visitor* visitor) {
	root_->accept(visitor, 0);
}

ostream& operator <<(ostream& os, const PHTree &tree) {
	os << "PH-Tree (dim=" << tree.dim_ << ", value length=" << tree.valueLength_ << ")" << endl;
	os << *tree.root_;
	return os;
}
