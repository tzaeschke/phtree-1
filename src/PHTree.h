/*
 * PHTree.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_PHTREE_H_
#define SRC_PHTREE_H_

#include <iostream>
#include <vector>

template <unsigned int DIM>
class Entry;
template <unsigned int DIM>
class Node;
template <unsigned int DIM>
class Visitor;
template <unsigned int DIM>
class RangeQueryIterator;

template <unsigned int DIM>
class PHTree {
	template <unsigned int D>
	friend std::ostream& operator<<(std::ostream& os, const PHTree<D>& tree);
public:
	PHTree(unsigned int valueLength);
	virtual ~PHTree();
	void insert(Entry<DIM>* e);
	std::pair<bool,int> lookup(Entry<DIM>* e);
	RangeQueryIterator<DIM>* rangeQuery(Entry<DIM>* lowerLeft, Entry<DIM>* upperRight);

	void accept(Visitor<DIM>* visitor);
	unsigned long* reserveSuffixBlocks(size_t nSuffixBlocks);
	void freeSuffixBlocks(unsigned long* suffixStartBlock, size_t nSuffixBlocks);

protected:
	// TODO no pointer for better locality
	Node<DIM>* root_;
	size_t valueLength_;
	vector<unsigned long> globalSuffixBlocks;
};

#include "nodes/LHC.h"
#include "util/DynamicNodeOperationsUtil.h"
#include "util/SpatialSelectionOperationsUtil.h"
#include "util/NodeTypeUtil.h"
#include <assert.h>

using namespace std;
#define DEBUG false

template <unsigned int DIM>
PHTree<DIM>::PHTree(unsigned int valueLength) {
	valueLength_ = valueLength;
	root_ = NodeTypeUtil::buildNode<DIM>(0, 0);
}

template <unsigned int DIM>
PHTree<DIM>::~PHTree() {
	root_->recursiveDelete();
	globalSuffixBlocks.clear();
}

template <unsigned int DIM>
void PHTree<DIM>::insert(Entry<DIM>* e) {
	assert (e->getBitLength() == valueLength_ && "value length of new entries must match the tree's");
	assert (e->getDimensions() == DIM && "entry dimension must match the tree dimension");

	if (DEBUG)
		cout << "inserting: " << *e << endl;

	Node<DIM>* updatedRoot = DynamicNodeOperationsUtil<DIM>::insert(e, root_, DIM, valueLength_);
	if (updatedRoot != root_) {
		delete root_;
		root_ = updatedRoot;
	}
}

template <unsigned int DIM>
std::pair<bool,int> PHTree<DIM>::lookup(Entry<DIM>* e) {
	assert (e->getBitLength() == valueLength_ && "value length of entries must match the tree's");
	assert (e->getDimensions() == DIM && "entry dimension must match the tree dimension");

	if (DEBUG)
		cout << "searching: " << *e << endl;
	return SpatialSelectionOperationsUtil::lookup<DIM>(e, root_, NULL);
}

template <unsigned int DIM>
RangeQueryIterator<DIM>* PHTree<DIM>::rangeQuery(Entry<DIM>* lowerLeft, Entry<DIM>* upperRight) {
	assert (lowerLeft->getBitLength() == valueLength_ && "value length of the lower left corner must match the tree's");
	assert (upperRight->getBitLength() == valueLength_ && "value length of the upper right corner must match the tree's");
	assert (lowerLeft->getDimensions() == DIM && upperRight->getDimensions() == DIM && "entry dimension must match the tree dimension");
	// TODO check of lower left and upper right corners are correctly set

	return root_->rangeQuery(lowerLeft, upperRight, 0, 0);
}

template <unsigned int DIM>
void PHTree<DIM>::accept(Visitor<DIM>* visitor) {
	root_->accept(visitor, 0);
}

template <unsigned int DIM>
unsigned long* PHTree<DIM>::reserveSuffixBlocks(size_t nSuffixBlocks) {
	unsigned long* startBlock = NULL;
	for (int i = 0; i < nSuffixBlocks; ++i) {
		globalSuffixBlocks.push_back(0);
		if (i == 0) {
			startBlock = &(globalSuffixBlocks[globalSuffixBlocks.size() - 1]);
		}
	}

	return startBlock;
}

template <unsigned int DIM>
void PHTree<DIM>::freeSuffixBlocks(unsigned long* suffixStartBlock, size_t nSuffixBlocks) {
	// TODO handle free!
}


template <unsigned int D>
ostream& operator <<(ostream& os, const PHTree<D> &tree) {
	os << "PH-Tree (dim=" << D << ", value length=" << tree.valueLength_ << ")" << endl;
	os << *tree.root_;
	return os;
}

#endif /* SRC_PHTREE_H_ */
