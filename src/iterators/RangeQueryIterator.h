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
	RangeQueryIterator(std::vector<std::pair<unsigned long, const Node<DIM>*>>* nodeStack,
			const Entry<DIM, WIDTH> lowerLeft,
			const Entry<DIM, WIDTH> upperRight);
	virtual ~RangeQueryIterator();

	Entry<DIM, WIDTH> next();
	bool hasNext() const;

private:
	static const size_t highestAddress = (1u << DIM) - 1;

	bool hasNext_;
	size_t currentIndex_;

	// stack of not fully traversed nodes
	std::stack<RangeQueryStackContent<DIM>> stack_;
	// relevant information for the currently processed node
	RangeQueryStackContent<DIM> currentContent;
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
	inline void createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength, bool nodeAndSuffixesFullyInRange);
	inline void goToNextValidSuffix();
};

#include <assert.h>
#include "nodes/NodeAddressContent.h"
#include "util/SpatialSelectionOperationsUtil.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>::RangeQueryIterator(vector<pair<unsigned long, const Node<DIM>*>>* visitedNodes,
		const Entry<DIM, WIDTH> lowerLeft, const Entry<DIM, WIDTH> upperRight) : hasNext_(true),
		currentIndex_(0), stack_(),
		currentValue(), lowerLeftCorner_(lowerLeft),
		upperRightCorner_(upperRight) {

#ifndef NDEBUG
	// validation only: lower left < upper right
	pair<unsigned long, unsigned long> comp = MultiDimBitset<DIM>::
				compareSmallerEqual(lowerLeftCorner_.values_, upperRightCorner_.values_, DIM * WIDTH, 0);
	assert ((comp.first == highestAddress) && "should be: lower left < upper right");
#endif

	assert (!visitedNodes->empty() && "at least the root node must have been visited");
	if (visitedNodes->empty()) {
		hasNext_ = false;
	} else {
		// the first node has to be the root node which does not have a prefix!
		createCurrentContent((*visitedNodes)[0].second, 0, false);
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
	while (!stack_.empty()) {
		RangeQueryStackContent<DIM> content = stack_.top();
		delete content.startIt_;
		delete content.endIt_;
		stack_.pop();
	}

	if (currentContent.startIt_)
		delete currentContent.startIt_;
	if (currentContent.endIt_)
		delete currentContent.endIt_;
}

template <unsigned int DIM, unsigned int WIDTH>
Entry<DIM, WIDTH> RangeQueryIterator<DIM, WIDTH>::next() {
	assert (hasNext());
	assert (currentAddressContent.exists && isInMaskRange(currentAddressContent.address)
		&& !currentAddressContent.hasSubnode && isSuffixInRange());
	assert (currentIndex_ < WIDTH);
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM)
			&& "there always need to be space for the last interleaved address");
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));

	// found a valid suffix in the range
	Entry<DIM, WIDTH> entry(currentValue, currentAddressContent.id);
	// copy the suffix into the entry
	MultiDimBitset<DIM>::pushBackValue(currentAddressContent.address, entry.values_, DIM * (WIDTH - currentIndex_ - 1));
	if (currentIndex_ < WIDTH - 1) {
		// need to copy the remaining suffix into the block
		const size_t suffixBits = DIM * (WIDTH - (currentIndex_ + 1));
		assert (suffixBits > 0);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, suffixBits));
		MultiDimBitset<DIM>::pushBackBitset(currentAddressContent.getSuffixStartBlock(), suffixBits, entry.values_, 0);
	}


#ifndef NDEBUG
	// validation only: is the entry contained in the current node
	assert (entry.id_ == currentContent.node_->lookup(currentAddressContent.address).id);

	// validation only: is the retrieved entry part of the tree?
	const Node<DIM>* rootNode;
	if (stack_.empty()) rootNode = currentContent.node_;
	else {
		// the root node is at the bottom of the stack
		stack<RangeQueryStackContent<DIM>> reverseStack;
		while (!stack_.empty()) {
			RangeQueryStackContent<DIM> top = stack_.top();
			rootNode = top.node_;
			reverseStack.push(top);
			stack_.pop();
		}
		while (!reverseStack.empty()) {
			stack_.push(reverseStack.top());
			reverseStack.pop();
		}
	}

	std::pair<bool, int> lookup = SpatialSelectionOperationsUtil<DIM, WIDTH>::lookup(entry, rootNode, NULL);
	assert (lookup.first && lookup.second == entry.id_);

	// validation only: lower left <= entry <= upper right
	// i.e. if in any dimension the inverse operation for < is not 0 there is an error
	pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
				compareSmallerEqual(lowerLeftCorner_.values_, entry.values_, DIM * WIDTH, 0);
	pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
				compareSmallerEqual(entry.values_, upperRightCorner_.values_, DIM * WIDTH, 0);
	assert (((lowerComp.first | lowerComp.second) == highestAddress) && "should be: lower left <= entry");
	assert (((upperComp.first | upperComp.second) == highestAddress) && "should be: entry <= upper right");
#endif

	++(*currentContent.startIt_);
	goToNextValidSuffix();

	return entry;
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::goToNextValidSuffix() {
	assert (hasNext_);

	do {
		while ((*currentContent.startIt_) == (*currentContent.endIt_) && hasNext_) {
			// ascend to a previous level if the end of the node was reached
			stepUp();
			++(*currentContent.startIt_);
			assert ((*currentContent.startIt_) <= (*currentContent.endIt_));
		}

		if (!hasNext_) break;
		currentAddressContent = *(*currentContent.startIt_);
		assert (currentAddressContent.exists);
		if (!isInMaskRange(currentAddressContent.address)) {
			++(*currentContent.startIt_);
		} else if (currentAddressContent.hasSubnode) {
			// descend to the next level in case of a subnode
			stepDown(currentAddressContent.subnode, currentAddressContent.address);
		} else if (isSuffixInRange()) {
			// found a suffix with a valid address
			break;
		} else {
			// the suffix was invalid so continue the search
			++(*currentContent.startIt_);
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
//	if (currentContent.nodeFullyContained) return true;

	assert (currentContent.upperMask_ < (1uL << DIM));
	assert (currentContent.lowerMask_ <= currentContent.upperMask_);

	const bool lowerMatch = (hcAddress | currentContent.lowerMask_) == hcAddress;
	const bool upperMatch = (hcAddress & currentContent.upperMask_) == hcAddress;
	return lowerMatch && upperMatch;
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::isSuffixInRange() {
	assert (currentAddressContent.exists && !currentAddressContent.hasSubnode
			&& isInMaskRange(currentAddressContent.address));
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));

//	if (currentContent.suffixesFullyContained) return true;

	// TODO no need to check if the current address is not the last address checked in the node
	// i.e. skip if current addresss < min(upper mask, highest filled node address);
//	if ((*currentContent.startIt_) != (*currentContent.endIt_)) return true;
//	if (currentContent.upperEqualToBoundary == 0) return true;

	// TODO check if suffix value in dimension i is smaller or equal to upper boundary
	// for those dimensions i that are set in upperEqualToBoundary

	// Verify if the final entry drops out of the range!
	MultiDimBitset<DIM>::pushBackValue(currentAddressContent.address, currentValue, DIM * (WIDTH - currentIndex_ - 1));
	const unsigned int suffixLength = WIDTH - currentIndex_ - 1;
	if (suffixLength > 0)
		MultiDimBitset<DIM>::pushBackBitset(currentAddressContent.getSuffixStartBlock(), DIM * suffixLength, currentValue, 0);

	pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
				compareSmallerEqual(lowerLeftCorner_.values_, currentValue, DIM * WIDTH, 0);
	pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
				compareSmallerEqual(currentValue, upperRightCorner_.values_, DIM * WIDTH, 0);
	MultiDimBitset<DIM>::removeHighestBits(currentValue, (1 + suffixLength) * DIM, (1 + suffixLength) * DIM);
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));
	if (((lowerComp.first | lowerComp.second) == highestAddress)
			&& ((upperComp.first | upperComp.second) == highestAddress)) {
		// the suffix is in the upper range
		// thus it will be in the next entry to be returned
		// TODO --> no need to clear the value
		return true;
	} else {
		// clear the suffix
		return false;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::stepUp() {
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));
	// TODO if several stack contents are skipped only a single remove operation is needed!
	if (stack_.empty()) {
		hasNext_ = false;
		assert (currentContent.prefixLength_ == 0
				&& "the last node should be the root which does not have a prefix");
		assert (currentIndex_ == 0);
	} else {
		// remove the prefix of the last node
		const size_t prefixLength = currentContent.prefixLength_;
		if (prefixLength > 0) {
			// remove current interleaved address (1) and prefix (+ prefixLength) bits (*DIM)
			currentIndex_ -= (prefixLength + 1);
			const unsigned int freeLsbBits = DIM * (WIDTH - currentIndex_);
			MultiDimBitset<DIM>::removeHighestBits(currentValue, freeLsbBits, (prefixLength + 1) * DIM);
			assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));
		} else {
			// only need to clear the current interleaved address
			MultiDimBitset<DIM>::clearValue(currentValue, DIM * (WIDTH - currentIndex_));
			currentIndex_ -= 1;
			assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));
		}

		// clear memory
		delete currentContent.startIt_;
		delete currentContent.endIt_;

		// restore the last contents from the top of the stack
		currentContent = stack_.top();
		stack_.pop();
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::stepDown(const Node<DIM>* nextNode, unsigned long hcAddress) {
	assert (nextNode && hcAddress < (1uL << DIM));
	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));

	// add the current interleaved address
	currentIndex_ += 1;
	MultiDimBitset<DIM>::pushBackValue(hcAddress, currentValue, (WIDTH - currentIndex_) * DIM);
	const size_t prefixLength = nextNode->getPrefixLength();
	if (prefixLength > 0) {
		// add the prefix of the next node to the current prefix
		currentIndex_ += prefixLength;
		MultiDimBitset<DIM>::pushBackBitset(nextNode->getFixPrefixStartBlock(), prefixLength * DIM,
				currentValue, (WIDTH - currentIndex_) * DIM);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));
	}

//TODO	const bool nodeAndSuffixesFullyInRange = currentContent.suffixesFullyContained;
	const bool nodeAndSuffixesFullyInRange = false;
	stack_.push(currentContent);
	createCurrentContent(nextNode, prefixLength, nodeAndSuffixesFullyInRange);

	assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, DIM * (WIDTH - currentIndex_)));
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength,
		bool nodeAndSuffixesFullyInRange) {
	assert (nextNode->getPrefixLength() == prefixLength);
	currentContent.node_ = nextNode;
	currentContent.prefixLength_ = prefixLength;

	bool fullSwipe = false;

	if (!nodeAndSuffixesFullyInRange) {
		// calculate the range masks for the next node
		const unsigned int ignoreNLowestBits = DIM * (WIDTH - currentIndex_ - 1);
		assert (ignoreNLowestBits >= 0 && ignoreNLowestBits <= WIDTH * DIM);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, ignoreNLowestBits + DIM));
		// msb       [interleaved format]          lsb
		// <-filled-><DIM><-------- ignored --------->
		// [ higher |00000|     lower node bits      ]
		// the current lower range is automatically se 0s
		pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
				compareSmallerEqual(lowerLeftCorner_.values_, currentValue, DIM * WIDTH, ignoreNLowestBits);

		// msb       [interleaved format]          lsb
		// <-filled-><DIM><-------- ignored --------->
		// [ higher |11111|      lower node bits     ]
		// add the highest possible address as an upper boundary of the node and remove it afterwards
		MultiDimBitset<DIM>::pushBackValue(highestAddress, currentValue, ignoreNLowestBits);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, ignoreNLowestBits));
		pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
				compareSmallerEqual(upperRightCorner_.values_, currentValue, DIM * WIDTH, ignoreNLowestBits);
		MultiDimBitset<DIM>::clearValue(currentValue, ignoreNLowestBits);
		assert (MultiDimBitset<DIM>::checkRangeUnset(currentValue, WIDTH * DIM, 0, ignoreNLowestBits + DIM));

		// lower mask: for i=0 to DIM - 1 do
		// 		lowerMask[i] = 0 <=> current value (in dimension i) is less or equal to the lower left corner (in dimension i)
		currentContent.lowerMask_ =  highestAddress & (~(lowerComp.first | lowerComp.second));
		// upper mask: for i=0 to DIM - 1 do
		// 		lowerMask[i] = 1 <=> current value (in dimension i) is higher or equal to upper right corner (in dimension i)
		currentContent.upperMask_ = highestAddress & ((~upperComp.first) | upperComp.second);
		assert (currentContent.lowerMask_ <= currentContent.upperMask_ && currentContent.upperMask_ < (1uL << DIM));

		fullSwipe = (currentContent.lowerMask_ == 0uL)
						&& (currentContent.upperMask_ == highestAddress);
		currentContent.nodeFullyContained = fullSwipe && upperComp.second == 0 && lowerComp.second == 0;
		currentContent.suffixesFullyContained = fullSwipe && upperComp.second == 0;
	} else {
		fullSwipe = true;
		currentContent.lowerMask_ = 0u;
		currentContent.upperMask_ = highestAddress;
		currentContent.nodeFullyContained = true;
		currentContent.suffixesFullyContained = true;
	}

	// calculate the iterators for the current node from the determined masks
	if (fullSwipe) {
		currentContent.startIt_ = nextNode->begin();
		currentContent.endIt_ = nextNode->end();
	} else {
		// start at the lower mask (= smallest possible interleaved address)
		currentContent.startIt_ = nextNode->it(currentContent.lowerMask_);
		// end before the upper mask (= highest possible interleaved address)
		currentContent.endIt_ = nextNode->it(currentContent.upperMask_ + 1);
		assert ((*currentContent.startIt_) <= (*currentContent.endIt_));
	}
}


#endif /* SRC_ITERATORS_RANGEQUERYITERATOR_H_ */
