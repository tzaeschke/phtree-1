/*
 * RangeQueryIterator.h
 *
 *  Created on: Mar 16, 2016
 *      Author: max
 */

#ifndef SRC_ITERATORS_RANGEQUERYITERATOR_H_
#define SRC_ITERATORS_RANGEQUERYITERATOR_H_

#include <stack>
#include "nodes/Node.h"
#include "iterators/NodeIterator.h"
#include "iterators/RangeQueryStackContent.h"
#include "util/MultiDimBitset.h"
#include "Entry.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM, unsigned int WIDTH>
class RangeQueryIterator {
public:
	// TODO add version that does not recreate the spatial data but only returns IDs
	RangeQueryIterator(std::vector<std::pair<unsigned long, const Node<DIM>*>>* nodeStack,
			const Entry<DIM, WIDTH>& lowerLeft,
			const Entry<DIM, WIDTH>& upperRight);
	virtual ~RangeQueryIterator();

	Entry<DIM, WIDTH> next();
	bool hasNext() const;

private:
	static const size_t highestAddress = (1u << DIM) - 1;

	bool hasNext_;
	size_t currentIndex_;
	size_t stackIndex_;

	// stack of not fully traversed nodes (max depth = max tree depth = bit width) and one guardian node
	RangeQueryStackContent<DIM> stack_[WIDTH + 1];
	// storage for bits from higher nodes (lower levels from stack)
	// Combined with a suffix this defines an entry.
	unsigned long currentValue[1 + (DIM * WIDTH - 1) / (sizeof (unsigned long) * 8)];
	// The address contents of the currently processed address in the currently processed node
	NodeAddressContent<DIM> currentAddressContent;

	const Entry<DIM, WIDTH> lowerLeftCorner_;
	const Entry<DIM, WIDTH> upperRightCorner_;


	void stepUp();
	void stepDown(const Node<DIM>* nextNode, unsigned long hcAddress);
	inline bool isInMaskRange(unsigned long hcAddress) const;
	inline bool isSuffixInRange();
	inline void createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength);
	inline void goToNextValidSuffix();
};

#include <assert.h>
#include "nodes/NodeAddressContent.h"
#include "util/SpatialSelectionOperationsUtil.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>::RangeQueryIterator(vector<pair<unsigned long, const Node<DIM>*>>* visitedNodes,
		const Entry<DIM, WIDTH>& lowerLeft, const Entry<DIM, WIDTH>& upperRight) : hasNext_(true),
		currentIndex_(0), stackIndex_(1), stack_(),
		currentValue(), lowerLeftCorner_(lowerLeft),
		upperRightCorner_(upperRight) {

#ifndef NDEBUG
	// validation only: lower left < upper right
	pair<unsigned long, unsigned long> comp = MultiDimBitset<DIM>::
				compareSmallerEqual(lowerLeftCorner_.values_, upperRightCorner_.values_, DIM * WIDTH, 0, highestAddress);
	assert ((comp.first == highestAddress) && "should be: lower left < upper right");
#endif

	assert (!visitedNodes->empty() && "at least the root node must have been visited");
	if (visitedNodes->empty()) {
		hasNext_ = false;
	} else {
		stack_[0].fullyContained = false;
		stack_[0].lowerContained = false;
		stack_[0].upperContained = false;
		// start: range window is fully included in domain
		stack_[0].lowerCompEqual = highestAddress;
		stack_[0].lowerCompSmaller = 0;
		stack_[0].upperCompEqual = highestAddress;
		stack_[0].upperCompSmaller = 0;
		// the first node has to be the root node which does not have a prefix!
		const Node<DIM>* root = (*visitedNodes)[0].second;
		createCurrentContent(root, 0);
		for (unsigned int i = 1; i < visitedNodes->size(); ++i) {
			const pair<unsigned long, const Node<DIM>*> nextNode = (*visitedNodes)[i];
			stepDown(nextNode.second, nextNode.first);
		}

		// thereby checks if there is any valid entry in the range
		goToNextValidSuffix();
	}
}

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>::~RangeQueryIterator() {
	for (unsigned i = 1; i < stackIndex_ + 1; ++i) {
		delete stack_[i].startIt_;
		delete stack_[i].endIt_;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
Entry<DIM, WIDTH> RangeQueryIterator<DIM, WIDTH>::next() {
	assert (hasNext());
	assert (currentAddressContent.exists && isInMaskRange(currentAddressContent.address)
		&& !currentAddressContent.hasSubnode && isSuffixInRange());
	assert (currentIndex_ < WIDTH);
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM, 0)
			&& "there always needs to be space for the last interleaved address");
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));

	// found a valid suffix in the range
	Entry<DIM, WIDTH> entry(currentValue, currentAddressContent.id);
	// copy the suffix into the entry
	MultiDimBitset<DIM>::pushBackValue(currentAddressContent.address, entry.values_, DIM * (WIDTH - currentIndex_ - 1));
	if (currentIndex_ < WIDTH - 1) {
		// need to copy the remaining suffix into the block
		const size_t suffixBits = DIM * (WIDTH - (currentIndex_ + 1));
		assert (suffixBits > 0);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, suffixBits, 0));
		MultiDimBitset<DIM>::pushBackBitset(currentAddressContent.getSuffixStartBlock(), suffixBits, entry.values_, 0);
	}


#ifndef NDEBUG
	// validation only: is the entry contained in the current node
	assert (entry.id_ == stack_[stackIndex_].node_->lookup(currentAddressContent.address).id);

	// validation only: is the retrieved entry part of the tree?
	const Node<DIM>* rootNode = stack_[1].node_;
	std::pair<bool, int> lookup = SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(entry, rootNode, NULL);
	assert (lookup.first && lookup.second == entry.id_);

	// validation only: lower left <= entry <= upper right
	// i.e. if in any dimension the inverse operation for < is not 0 there is an error
	pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
				compareSmallerEqual(lowerLeftCorner_.values_, entry.values_, DIM * WIDTH, 0, highestAddress);
	pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
				compareSmallerEqual(entry.values_, upperRightCorner_.values_, DIM * WIDTH, 0, highestAddress);
	assert (((lowerComp.first | lowerComp.second) == highestAddress) && "should be: lower left <= entry");
	assert (((upperComp.first | upperComp.second) == highestAddress) && "should be: entry <= upper right");
#endif

	++(*(stack_[stackIndex_]).startIt_);
	goToNextValidSuffix();

	return entry;
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::goToNextValidSuffix() {
	assert (hasNext_);

	do {
		while ((*(stack_[stackIndex_]).startIt_) == (*(stack_[stackIndex_]).endIt_) && hasNext_) {
			// ascend to a previous level if the end of the node was reached
			stepUp();
			++(*(stack_[stackIndex_]).startIt_);
			assert ((*(stack_[stackIndex_]).startIt_) <= (*(stack_[stackIndex_]).endIt_));
		}

		if (!hasNext_) break;
		currentAddressContent = *(*(stack_[stackIndex_]).startIt_);
		assert (currentAddressContent.exists);
		if (!isInMaskRange(currentAddressContent.address)) {
			++(*(stack_[stackIndex_]).startIt_);
		} else if (currentAddressContent.hasSubnode) {
			// descend to the next level in case of a subnode
			stepDown(currentAddressContent.subnode, currentAddressContent.address);
		} else if (isSuffixInRange()) {
			// found a suffix with a valid address
			break;
		} else {
			// the suffix was invalid so continue the search
			++(*(stack_[stackIndex_]).startIt_);
		}
	} while (hasNext_);

	assert (!hasNext_ || (currentAddressContent.exists && isInMaskRange(currentAddressContent.address) && !currentAddressContent.hasSubnode));
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::hasNext() const {
	return hasNext_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::isInMaskRange(unsigned long hcAddress) const {

	assert (stack_[stackIndex_].upperMask_ < (1uL << DIM));
	assert (stack_[stackIndex_].lowerMask_ <= stack_[stackIndex_].upperMask_);
	assert (stack_[stackIndex_].lowerMask_ <= hcAddress && hcAddress <= stack_[stackIndex_].upperMask_);

	if (stack_[stackIndex_].fullyContained) return true;

	const bool lowerMatch = (hcAddress | stack_[stackIndex_].lowerMask_) == hcAddress;
	const bool upperMatch = (hcAddress & stack_[stackIndex_].upperMask_) == hcAddress;
	return lowerMatch && upperMatch;
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::isSuffixInRange() {
	assert (currentAddressContent.exists && !currentAddressContent.hasSubnode
			&& isInMaskRange(currentAddressContent.address));
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));

	if (stack_[stackIndex_].fullyContained) { return true; }

	// Verify if the final entry drops out of the range!
	const unsigned int suffixLength = WIDTH - currentIndex_ - 1;
	MultiDimBitset<DIM>::pushBackValue(currentAddressContent.address, currentValue, DIM * (WIDTH - currentIndex_ - 1));
	if (suffixLength > 0)
		MultiDimBitset<DIM>::pushBackBitset(currentAddressContent.getSuffixStartBlock(), DIM * suffixLength, currentValue, 0);

	bool suffixContained = true;
	// validate: range lower left <= entry (suffix)
	if (!stack_[stackIndex_].lowerContained) {
		pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
					compareSmallerEqual(lowerLeftCorner_.values_, currentValue, DIM * WIDTH, 0, highestAddress);
		suffixContained = (lowerComp.first | lowerComp.second) == highestAddress;
	}

	// validate: entry (suffix) <= range upper right
	if (suffixContained && !stack_[stackIndex_].upperContained) {
		pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
					compareSmallerEqual(currentValue, upperRightCorner_.values_, DIM * WIDTH, 0, highestAddress);
		suffixContained = (upperComp.first | upperComp.second) == highestAddress;
	}

	if (suffixLength > 0) {
		MultiDimBitset<DIM>::removeHighestBits(currentValue, (1 + suffixLength) * DIM, (1 + suffixLength) * DIM);
	} else {
		MultiDimBitset<DIM>::clearValue(currentValue, DIM * (WIDTH - currentIndex_ - 1));
	}

	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
	if (suffixContained) {
		// the suffix is in the upper range
		// thus it will be in the next entry to be returned
		// TODO --> no need to clear the value
		return true;
	} else {
		return false;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::stepUp() {
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
	assert ((*(stack_[stackIndex_]).startIt_) == (*(stack_[stackIndex_]).endIt_));
	// TODO if several stack contents are skipped only a single remove operation is needed!
	if (stackIndex_ == 1) {
		hasNext_ = false;
		assert (stack_[stackIndex_].prefixLength_ == 0
				&& "the last node should be the root which does not have a prefix");
		assert (currentIndex_ == 0);
	} else {
		// remove the prefix of the last node
		const size_t prefixLength = stack_[stackIndex_].prefixLength_;
		if (prefixLength > 0) {
			// remove current interleaved address (1) and prefix (+ prefixLength) bits (*DIM)
			currentIndex_ -= (prefixLength + 1);
			const unsigned int freeLsbBits = DIM * (WIDTH - currentIndex_);
			MultiDimBitset<DIM>::removeHighestBits(currentValue, freeLsbBits, (prefixLength + 1) * DIM);
			assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
		} else {
			// only need to clear the current interleaved address
			MultiDimBitset<DIM>::clearValue(currentValue, DIM * (WIDTH - currentIndex_));
			currentIndex_ -= 1;
			assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
		}

		// clear memory
		// TODO do not remove iterators but instead reset them in the next run if possible
		delete stack_[stackIndex_].startIt_;
		delete stack_[stackIndex_].endIt_;

		--stackIndex_;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::stepDown(const Node<DIM>* nextNode, unsigned long hcAddress) {
	assert (nextNode && hcAddress < (1uL << DIM));
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
	assert ((*(stack_[stackIndex_]).startIt_) < (*(stack_[stackIndex_]).endIt_));

	// add the current interleaved address
	currentIndex_ += 1;
	MultiDimBitset<DIM>::pushBackValue(hcAddress, currentValue, (WIDTH - currentIndex_) * DIM);
	const size_t prefixLength = nextNode->getPrefixLength();
	if (prefixLength > 0) {
		// add the prefix of the next node to the current prefix
		currentIndex_ += prefixLength;
		MultiDimBitset<DIM>::pushBackBitset(nextNode->getFixPrefixStartBlock(), prefixLength * DIM,
				currentValue, (WIDTH - currentIndex_) * DIM);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
	}

	// puts a duplicate on the stack
	++stackIndex_;
	createCurrentContent(nextNode, prefixLength);

	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, DIM * (WIDTH - currentIndex_), 0));
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength) {
	assert (nextNode->getPrefixLength() == prefixLength);
	stack_[stackIndex_].node_ = nextNode;
	stack_[stackIndex_].prefixLength_ = prefixLength;

	bool fullSwipe = true;

	if (!stack_[stackIndex_ - 1].fullyContained) {
		// calculate the range masks for the next node
		const unsigned int ignoreNLowestBits = DIM * (WIDTH - currentIndex_ - 1);
		assert (ignoreNLowestBits >= 0 && ignoreNLowestBits <= WIDTH * DIM);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, ignoreNLowestBits + DIM, 0));

		if (!stack_[stackIndex_ - 1].lowerContained) {
			// msb       [interleaved format]          lsb
			// <-filled-><DIM><-------- ignored --------->
			// [ higher |00000|     lower node bits      ]
			// the current lower range is automatically se 0s
			pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
					compareSmallerEqual(lowerLeftCorner_.values_, currentValue, DIM * WIDTH, ignoreNLowestBits, highestAddress);
			stack_[stackIndex_].lowerCompSmaller = lowerComp.first;
			stack_[stackIndex_].lowerCompEqual = lowerComp.second;
			// lower mask: for i=0 to DIM - 1 do
			// 		lowerMask[i] = 0 <=> current value (in dimension i) is less or equal to the lower left corner (in dimension i)
			stack_[stackIndex_].lowerMask_ =  highestAddress & (~(lowerComp.first | lowerComp.second));
			stack_[stackIndex_].lowerContained = (stack_[stackIndex_].lowerCompEqual == 0)
					&& (stack_[stackIndex_].lowerCompSmaller == highestAddress);
		} else {
			stack_[stackIndex_].lowerContained = true;
			stack_[stackIndex_].lowerMask_ = 0;
			stack_[stackIndex_].lowerCompEqual = 0;
			stack_[stackIndex_].lowerCompSmaller = highestAddress;
		}

		if (!stack_[stackIndex_ - 1].upperContained) {
			// msb       [interleaved format]          lsb
			// <-filled-><DIM><-------- ignored --------->
			// [ higher |11111|      lower node bits     ]
			// add the highest possible address as an upper boundary of the node and remove it afterwards
			MultiDimBitset<DIM>::pushBackValue(highestAddress, currentValue, ignoreNLowestBits);
			assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, ignoreNLowestBits, 0));
			pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
					compareSmallerEqual(upperRightCorner_.values_, currentValue, DIM * WIDTH, ignoreNLowestBits, highestAddress);
			MultiDimBitset<DIM>::clearValue(currentValue, ignoreNLowestBits);
			assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, ignoreNLowestBits + DIM, 0));
			stack_[stackIndex_].upperCompSmaller = upperComp.first;
			stack_[stackIndex_].upperCompEqual = upperComp.second;
			// upper mask: for i=0 to DIM - 1 do
			// 		lowerMask[i] = 1 <=> current value (in dimension i) is higher or equal to upper right corner (in dimension i)
			stack_[stackIndex_].upperMask_ = highestAddress & ((~upperComp.first) | upperComp.second);
			stack_[stackIndex_].upperContained = (stack_[stackIndex_].upperCompEqual == 0)
					&& (stack_[stackIndex_].upperCompSmaller == 0);
		} else {
			stack_[stackIndex_].upperContained = true;
			stack_[stackIndex_].upperMask_ = highestAddress;
			stack_[stackIndex_].upperCompEqual = 0;
			stack_[stackIndex_].upperCompSmaller = 0;
		}

		fullSwipe = (stack_[stackIndex_].lowerMask_ == 0uL)
				&& (stack_[stackIndex_].upperMask_ == highestAddress);
		stack_[stackIndex_].fullyContained = fullSwipe
				&& stack_[stackIndex_].upperCompEqual == 0
				&& stack_[stackIndex_].lowerCompEqual == 0;
	} else {
		stack_[stackIndex_].fullyContained = true;
		stack_[stackIndex_].lowerMask_ = 0;
		stack_[stackIndex_].upperMask_ = highestAddress;
	}

	// calculate the iterators for the current node from the determined masks
	if (fullSwipe) {
		assert (stack_[stackIndex_].lowerMask_  == 0);
		assert (stack_[stackIndex_].upperMask_ == highestAddress);
		stack_[stackIndex_].startIt_ = nextNode->begin();
		stack_[stackIndex_].endIt_ = nextNode->end();
	} else {
		// start at the lower mask (= smallest possible interleaved address)
		stack_[stackIndex_].startIt_ = nextNode->it(stack_[stackIndex_].lowerMask_);
		// end before the upper mask (= highest possible interleaved address)
		stack_[stackIndex_].endIt_ = nextNode->it(stack_[stackIndex_].upperMask_ + 1);
	}

	assert (stack_[stackIndex_].lowerMask_ <= stack_[stackIndex_].upperMask_
			&& stack_[stackIndex_].upperMask_ < (1uL << DIM));
	assert ((*(stack_[stackIndex_]).startIt_) <= (*(stack_[stackIndex_]).endIt_));
	assert (stack_[stackIndex_].lowerMask_ <= stack_[stackIndex_].startIt_->getAddress());
	assert (stack_[stackIndex_].upperMask_ < stack_[stackIndex_].endIt_->getAddress());
	// if the lower and upper range is contained than the node is definitely fully contained
	// notice that the inversion is not always true
	assert (!(stack_[stackIndex_].lowerContained && stack_[stackIndex_].upperContained) || stack_[stackIndex_].fullyContained);
}


#endif /* SRC_ITERATORS_RANGEQUERYITERATOR_H_ */
