/*
 * PHTree.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "PHTree.h"
#include "AHC.h"

PHTree::PHTree(int dim, int valueLength) {
	valueLength_ = valueLength;
	dim_ = dim;
	root_ = new AHC(dim, valueLength);
}

PHTree::~PHTree() {
	delete root_;
}

void PHTree::insert(Entry* e) {
	cout << "inserting: " << *e << endl;
	root_->insert(e, 0, 0);
}

bool PHTree::lookup(Entry* e) {
	cout << "searching: " << *e << endl;
	return root_->lookup(e, 0, 0);
}
