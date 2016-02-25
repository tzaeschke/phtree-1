/*
 * PHTree.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_PHTREE_H_
#define SRC_PHTREE_H_

#include "Node.h"
#include "Entry.h"

using namespace std;

class PHTree {
public:
	Node* root_;
	int dim_; // dimensions (k)
	int valueLength_;

	PHTree(int dim, int valueLength);

	virtual ~PHTree();

	void insert(Entry* e);

	bool lookup(Entry* e);
};

#endif /* SRC_PHTREE_H_ */
