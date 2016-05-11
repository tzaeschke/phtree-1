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

template <unsigned int DIM, unsigned int WIDTH>
class Entry;
template <unsigned int DIM>
class Node;
template <unsigned int DIM>
class Visitor;
template <unsigned int DIM>
class SizeVisitor;
template <unsigned int DIM, unsigned int WIDTH>
class RangeQueryIterator;
template <unsigned int SUFF_PER_BLOCK>
class SuffixBlock;

template <unsigned int DIM, unsigned int WIDTH>
class PHTree {
	template <unsigned int D>
	friend class SizeVisitor;
	template <unsigned int D, unsigned int W>
	friend std::ostream& operator<<(std::ostream& os, const PHTree<D, W>& tree);
public:
	PHTree();
	virtual ~PHTree();
	void insert(const Entry<DIM, WIDTH>* e);
	std::pair<bool,int> lookup(const Entry<DIM, WIDTH>* e);
	RangeQueryIterator<DIM, WIDTH>* rangeQuery(Entry<DIM, WIDTH>* lowerLeft, Entry<DIM, WIDTH>* upperRight);

	void accept(Visitor<DIM>* visitor);
	unsigned long* reserveSuffixSpace(size_t nSuffixBits);
	void freeSuffixSpace(unsigned long* suffixStartBlock, size_t nSuffixBits);

protected:
	// TODO no pointer for better locality
	Node<DIM>* root_;
	// TODO whats the best size for each suffix block?
	SuffixBlock<50>* firstSuffixBlock;
	SuffixBlock<50>* currentSuffixBlock;
};

#include <assert.h>
#include "nodes/LHC.h"
#include "util/DynamicNodeOperationsUtil.h"
#include "util/SpatialSelectionOperationsUtil.h"
#include "util/NodeTypeUtil.h"
#include "util/SuffixBlock.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
PHTree<DIM, WIDTH>::PHTree() {
	root_ = NodeTypeUtil::buildNode<DIM>(0, 0);
	firstSuffixBlock = new SuffixBlock<50>();
	currentSuffixBlock = firstSuffixBlock;
}

template <unsigned int DIM, unsigned int WIDTH>
PHTree<DIM, WIDTH>::~PHTree() {
	root_->recursiveDelete();
	delete firstSuffixBlock;
}

template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>* e) {
	#ifdef PRINT
		cout << "inserting: " << (*e) << endl;
	#endif

	Node<DIM>* updatedRoot = DynamicNodeOperationsUtil<DIM, WIDTH>::insert(e, root_, this);
	if (updatedRoot != root_) {
		delete root_;
		root_ = updatedRoot;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
std::pair<bool,int> PHTree<DIM, WIDTH>::lookup(const Entry<DIM, WIDTH>* e) {
	#ifdef PRINT
		cout << "searching: " << *e << endl;
	#endif
	return SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(e, root_, NULL);
}

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>* PHTree<DIM, WIDTH>::rangeQuery(Entry<DIM, WIDTH>* lowerLeft, Entry<DIM, WIDTH>* upperRight) {
	// TODO check if lower left and upper right corners are correctly set
	//return root_->rangeQuery(lowerLeft, upperRight, 0, 0);
	return NULL;
}

template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::accept(Visitor<DIM>* visitor) {
	(*visitor).template visit<WIDTH>(this);
	root_->accept(visitor, 0);
}

template <unsigned int DIM, unsigned int WIDTH>
unsigned long* PHTree<DIM, WIDTH>::reserveSuffixSpace(size_t nSuffixBits) {
	const size_t nSuffixBlocks = (nSuffixBits > 0)? 1 + ((nSuffixBits - 1) / (sizeof (unsigned long) * 8)) : 0;
	return currentSuffixBlock->reserveSuffixBlocks(nSuffixBlocks, &currentSuffixBlock);
}

template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::freeSuffixSpace(unsigned long* suffixStartBlock, size_t nSuffixBits) {
	// TODO handle free!
}


template <unsigned int D, unsigned int W>
ostream& operator <<(ostream& os, const PHTree<D, W> &tree) {
	os << "PH-Tree (dim=" << D << ", value length=" << W << ")" << endl;
	tree.root_->output(os, 0, 0, W);
	return os;
}

#endif /* SRC_PHTREE_H_ */
