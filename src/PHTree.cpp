/*
 * PHTree.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "PHTree.h"
#include "AHC.h"

using namespace std;
#define DEBUG false

PHTree::PHTree(int dim, int valueLength) {
	valueLength_ = valueLength;
	dim_ = dim;
	root_ = new AHC(dim, valueLength);
}

PHTree::~PHTree() {
	delete root_;
}

void PHTree::insert(Entry* e) {
	if (DEBUG)
		cout << "inserting: " << *e << endl;
	root_->insert(e, 0, 0);
}

bool PHTree::lookup(Entry* e) {
	if (DEBUG)
		cout << "searching: " << *e << endl;
	return root_->lookup(e, 0, 0);
}

ostream& operator <<(ostream& os, const PHTree &tree) {
	os << "PH-Tree (dim=" << tree.dim_ << ", value length=" << tree.valueLength_ << ")" << endl;
	os << *tree.root_;
	return os;
}
