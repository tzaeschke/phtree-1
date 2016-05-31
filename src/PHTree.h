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
	template <unsigned int D, unsigned int W>
	friend class DynamicNodeOperationsUtil;
public:
	PHTree();
	virtual ~PHTree();
	void insert(const Entry<DIM, WIDTH>& e);
	void insert(const std::vector<unsigned long>& values, int id);
	void insertHyperRect(const std::vector<unsigned long>& lowerLeftValues, const std::vector<unsigned long>& upperRightValues, int id);
	std::pair<bool,int> lookup(const Entry<DIM, WIDTH>& e) const;
	std::pair<bool,int> lookup(const std::vector<unsigned long>& values) const;
	std::pair<bool,int> lookupHyperRect(const std::vector<unsigned long>& lowerLeftValues, const std::vector<unsigned long>& upperRightValues) const;
	RangeQueryIterator<DIM, WIDTH>* rangeQuery(const Entry<DIM, WIDTH>& lowerLeft, const Entry<DIM, WIDTH>& upperRight) const;
	RangeQueryIterator<DIM, WIDTH>* rangeQuery(const std::vector<unsigned long>& lowerLeftValues, const std::vector<unsigned long>& upperRightValues) const;
	RangeQueryIterator<DIM, WIDTH>* rangeQueryHyperRect(const std::vector<unsigned long>& lowerLeftValues, const std::vector<unsigned long>& upperRightValues) const;

	void accept(Visitor<DIM>* visitor);
	unsigned long* reserveSuffixSpace(size_t nSuffixBits);
	void freeSuffixSpace(unsigned long* suffixStartBlock, size_t nSuffixBits);

private:
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
	root_ = NodeTypeUtil<DIM>::buildNode(0, 1);
	firstSuffixBlock = new SuffixBlock<50>();
	currentSuffixBlock = firstSuffixBlock;
}

template <unsigned int DIM, unsigned int WIDTH>
PHTree<DIM, WIDTH>::~PHTree() {
	root_->recursiveDelete();
	delete firstSuffixBlock;
}

template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>& e) {
	#ifdef PRINT
		cout << "inserting: " << e << endl;
	#endif

	DynamicNodeOperationsUtil<DIM, WIDTH>::insert(e, root_, *this);
}

template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::insert(const vector<unsigned long>& values, int id) {
	assert (values.size() == DIM);
	const Entry<DIM, WIDTH> entry(values, id);
	insert(entry);
}

template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::insertHyperRect(
		const vector<unsigned long>& lowerLeftValues,
		const vector<unsigned long>& upperRightValues, int id) {
	assert (DIM % 2 == 0);
	assert (lowerLeftValues.size() == upperRightValues.size());
	assert (lowerLeftValues.size() + upperRightValues.size() == DIM);

	vector<unsigned long> combinedValues(DIM);
	for (unsigned i = 0; i < DIM / 2; ++i) {
		assert (lowerLeftValues[i] <= upperRightValues[i]);
		combinedValues[i] = lowerLeftValues[i];
		combinedValues[i + DIM / 2] = upperRightValues[i];
	}

	insert(combinedValues, id);
}

template <unsigned int DIM, unsigned int WIDTH>
pair<bool,int> PHTree<DIM, WIDTH>::lookup(const Entry<DIM, WIDTH>& e) const {
	#ifdef PRINT
		cout << "searching: " << e << endl;
	#endif
	return SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(e, root_, NULL);
}

template <unsigned int DIM, unsigned int WIDTH>
pair<bool,int> PHTree<DIM, WIDTH>::lookup(const std::vector<unsigned long>& values) const {
	const Entry<DIM, WIDTH> entry(values, 0);
	return lookup(entry);
}

template<unsigned int DIM, unsigned int WIDTH>
pair<bool, int> PHTree<DIM, WIDTH>::lookupHyperRect(
		const std::vector<unsigned long>& lowerLeftValues,
		const std::vector<unsigned long>& upperRightValues) const {
	assert (DIM % 2 == 0);
	assert(lowerLeftValues.size() == upperRightValues.size());
	assert(lowerLeftValues.size() + upperRightValues.size() == DIM);

	vector<unsigned long> combinedValues(DIM);
	for (unsigned i = 0; i < DIM / 2; ++i) {
		combinedValues[i] = lowerLeftValues[i];
		combinedValues[i + DIM / 2] = upperRightValues[i];
	}

	return lookup(combinedValues);
}


template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>* PHTree<DIM, WIDTH>::rangeQuery(const Entry<DIM, WIDTH>& lowerLeft,
		const Entry<DIM, WIDTH>& upperRight) const {
	// TODO check if lower left and upper right corners are correctly set
	vector<pair<unsigned long, const Node<DIM>*>>* visitedNodes = new vector<pair<unsigned long, const Node<DIM>*>>();
	SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(lowerLeft, root_, visitedNodes);
	RangeQueryIterator<DIM, WIDTH>* it = new RangeQueryIterator<DIM, WIDTH>(visitedNodes, lowerLeft, upperRight);

	return it;
}

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>* PHTree<DIM, WIDTH>::rangeQuery(
		const vector<unsigned long>& lowerLeftValues,
		const vector<unsigned long>& upperRightValues) const {
	const Entry<DIM, WIDTH> lowerLeft(lowerLeftValues, 0);
	const Entry<DIM, WIDTH> upperRight(upperRightValues, 0);
	return rangeQuery(lowerLeft, upperRight);
}

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>* PHTree<DIM, WIDTH>::rangeQueryHyperRect(
		const vector<unsigned long>& lowerLeftValues,
		const vector<unsigned long>& upperRightValues) const {
	assert (DIM % 2 == 0);
	assert ((2 * lowerLeftValues.size() == DIM) && (2 * upperRightValues.size() == DIM));

	// range query for k-dim hyper rectangles as 2k-dim points:
	// <--- k --->  <-- k -->   <-- k --> <--- k --->
	// (-inf, -inf, ll1, ll2) - (ur1, ur2, +inf, +inf)
	vector<unsigned long> lowerLeftHyperRect(DIM);
	vector<unsigned long> upperRightHyperRect(DIM);
	// set lower half of the values
	for (unsigned k = 0; k <  DIM / 2; ++k) {
		assert (lowerLeftValues[k] <= upperRightValues[k]);
		// with unsigned values 0 is the lowest possible value
		lowerLeftHyperRect[k] = 0;
		upperRightHyperRect[k] = upperRightValues[k];
	}

	// with unsigned values -1 is equal to the highest possible value
	const unsigned long max = (1uL << WIDTH) - 1;
	// set upper half of the values
	for (unsigned k = DIM / 2; k < DIM; ++k) {
		lowerLeftHyperRect[k] = lowerLeftValues[k - DIM / 2];
		upperRightHyperRect[k] = max;
	}

	return rangeQuery(lowerLeftHyperRect, upperRightHyperRect);
}


template <unsigned int DIM, unsigned int WIDTH>
void PHTree<DIM, WIDTH>::accept(Visitor<DIM>* visitor) {
	(*visitor).template visit<WIDTH>(this);
	root_->accept(visitor, 0, 0);
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
