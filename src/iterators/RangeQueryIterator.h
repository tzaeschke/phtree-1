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
	inline bool isSuffixInUpperRange();
	inline void createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength);
};

#include <assert.h>
#include "nodes/NodeAddressContent.h"

using namespace std;

// TODO also provide addresses per node that were followed during the lookup!
template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>::RangeQueryIterator(vector<pair<unsigned long, const Node<DIM>*>>* visitedNodes,
		const Entry<DIM, WIDTH> lowerLeft, const Entry<DIM, WIDTH> upperRight) : hasNext_(true),
		currentIndex_(0), stack_(),
		currentValue(), lowerLeftCorner_(lowerLeft),
		upperRightCorner_(upperRight) {

	assert (!visitedNodes->empty() && "at least the root node must have been visited");
	if (visitedNodes->empty()) {
		hasNext_ = false;
	} else {
		// the first node has to be the root node which does not have a prefix!
		createCurrentContent((*visitedNodes)[0].second, 0);
		for (unsigned int i = 1; i < visitedNodes->size(); ++i) {
			const pair<unsigned long, const Node<DIM>*> nextNode = (*visitedNodes)[i];
			stepDown(nextNode.second, nextNode.first);
		}

		currentAddressContent = *(*currentContent.startIt_);
	}
}

template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>::~RangeQueryIterator() {
	while (!stack_.empty()) {
		stepUp();
	}
}

template <unsigned int DIM, unsigned int WIDTH>
Entry<DIM, WIDTH> RangeQueryIterator<DIM, WIDTH>::next() {
	assert (hasNext());
	assert (currentAddressContent.exists && isInMaskRange(currentAddressContent.address));

	// extract the next entry from a suffix
	// (potentially need to descend into subnodes)
		// ascend to the next subnode with a suffix
		while (currentAddressContent.hasSubnode) {
			assert(currentAddressContent.exists && isInMaskRange(currentAddressContent.address));
			// TODO how to remove the current stack content in case the last address was reached because than there is no need to ascend to it again!
			stepDown(currentAddressContent.subnode, currentAddressContent.address);
			currentAddressContent = *(*currentContent.startIt_);
		}
		assert (isSuffixInUpperRange());

	// found a valid suffix in the range
	Entry<DIM, WIDTH> entry(currentValue, currentAddressContent.id);
	// copy the suffix into the entry
	MultiDimBitset<DIM>::pushBackValue(currentAddressContent.address, entry.values_, DIM * (WIDTH - currentIndex_ - 1));
	if (currentIndex_ < WIDTH - 1) {
		// need to copy the remaining suffix into the block
		const size_t suffixBits = DIM * (WIDTH - (currentIndex_ + 1));
		MultiDimBitset<DIM>::pushBackBitset(currentAddressContent.suffixStartBlock, suffixBits, entry.values_, 0);
	}

	// iterate to the next valid address (might be in a higher node)
	do {
		++(*currentContent.startIt_);
		if ((*currentContent.startIt_) == (*currentContent.endIt_) && hasNext_) {
			// reached the end of the range in the node so ascend to a higher node
			stepUp();
			++(*currentContent.startIt_);
		}

		currentAddressContent = *(*currentContent.startIt_);
		// go to next valid entry in the node's mask range
		while (hasNext_ && (*currentContent.startIt_) != (*currentContent.endIt_) && !isInMaskRange(currentAddressContent.address)) {
			++(*currentContent.startIt_);
			currentAddressContent = *(*currentContent.startIt_);
		}

		// repeat if:
		// - the end of the current node was reached
		// - the current value is valid but it holds a suffix that drops out of the range
	} while (hasNext_ && ((*currentContent.startIt_) == (*currentContent.endIt_)
			|| (!currentAddressContent.hasSubnode && !isSuffixInUpperRange())));

	hasNext_ = hasNext_ && isInMaskRange(currentAddressContent.address);
	return entry;
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::hasNext() const {
	return hasNext_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::isInMaskRange(unsigned long hcAddress) const {

	assert (currentContent.upperMask_ < (1uL << DIM));
	assert (currentContent.lowerMask_ <= currentContent.upperMask_);

	const bool lowerMatch = (hcAddress | currentContent.lowerMask_) == hcAddress;
	const bool upperMatch = (hcAddress & currentContent.upperMask_) == hcAddress;
	return lowerMatch && upperMatch;
}

template <unsigned int DIM, unsigned int WIDTH>
bool RangeQueryIterator<DIM, WIDTH>::isSuffixInUpperRange() {
	assert (currentAddressContent.exists && !currentAddressContent.hasSubnode
			&& isInMaskRange(currentAddressContent.address));
	// TODO no need to check if the current address is not the last address checked in the node
	// i.e. skip if current addresss < min(upper mask, highest filled node address);
//	if ((*currentContent.startIt_) != (*currentContent.endIt_)) return true;
	if (currentContent.upperEqualToBoundary == 0) return true;

	// TODO check if suffix value in dimension i is smaller or equal to upper boundary
	// for those dimensions i that are set in upperEqualToBoundary

	// varifiy if the final entry drops out of the range!
	MultiDimBitset<DIM>::pushBackValue(currentAddressContent.address, currentValue, DIM * (WIDTH - currentIndex_ - 1));
	const unsigned int suffixLength = WIDTH - currentIndex_ - 1;
	MultiDimBitset<DIM>::pushBackBitset(currentAddressContent.suffixStartBlock, DIM * suffixLength, currentValue, 0);
	pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
				compareSmallerEqual(upperRightCorner_.values_, currentValue, DIM * WIDTH, 0);
	if ((upperComp.first | upperComp.second) == highestAddress) {
		// the suffix is in the upper range
		// thus it will be in the next entry to be returned
		// --> no need to clear the value
		return true;
	} else {
		// clear the suffix
		MultiDimBitset<DIM>::removeHighestBits(currentValue, (1 + suffixLength) * DIM, (1 + suffixLength) * DIM);
		return false;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::stepUp() {
	// TODO if several stack contents are skipped only a single remove operation is needed!
	if (stack_.empty()) {
		hasNext_ = false;
	} else {
		// remove the prefix of the last node
		const size_t prefixLength = currentContent.prefixLength_;
		if (prefixLength > 0) {
			// remove current interleaved address and prefix bits
			currentIndex_ -= (prefixLength + 1);
			const unsigned int lsbBits = DIM * (WIDTH - currentIndex_ - 1);
			MultiDimBitset<DIM>::removeHighestBits(currentValue, lsbBits, (1 + prefixLength) * DIM);
		} else {
			// only need to clear the current interleaved address
			MultiDimBitset<DIM>::clearValue(currentValue, currentIndex_ * DIM);
			currentIndex_ -= 1;
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

	// add the prefix of the next node to the current prefix
	currentIndex_ += 1;
	MultiDimBitset<DIM>::pushBackValue(hcAddress, currentValue, (WIDTH - currentIndex_) * DIM);
	const size_t prefixLength = nextNode->getPrefixLength();
	if (prefixLength > 0) {
		currentIndex_ += prefixLength;
		MultiDimBitset<DIM>::pushBackBitset(nextNode->getFixPrefixStartBlock(), prefixLength * DIM,
				currentValue, (WIDTH - currentIndex_) * DIM);
	}

	stack_.push(currentContent);
	createCurrentContent(nextNode, prefixLength);
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength) {
	assert (nextNode->getPrefixLength() == prefixLength);
	currentContent.node_ = nextNode;
	currentContent.prefixLength_ = prefixLength;

	// calculate the range masks for the next node
	const unsigned int ignoreNLowestBits = DIM * (WIDTH - currentIndex_ - 1);
	assert (ignoreNLowestBits >= 0 && ignoreNLowestBits <= WIDTH * DIM);
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
	pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
			compareSmallerEqual(upperRightCorner_.values_, currentValue, DIM * WIDTH, ignoreNLowestBits);
	MultiDimBitset<DIM>::clearValue(currentValue, ignoreNLowestBits);

	// lower mask: for i=0 to DIM - 1 do
	// 		lowerMask[i] = 0 <=> current value (in dimension i) is less or equal to lower left corner (in dimension i)
	//		lowerMask[i] = 1 <=> current value is not smaller and not equal to lower left corner
	currentContent.lowerMask_ =  highestAddress & ((~lowerComp.first) & (~lowerComp.second));
	// upper mask: for i=0 to DIM - 1 do
	// 		lowerMask[i] = 1 <=> current value (in dimension i) is higher or equal to upper right corner (in dimension i)
	currentContent.upperMask_ = ((~upperComp.first) & highestAddress);
	currentContent.upperEqualToBoundary = upperComp.second;

	// start at the lower mask (= smallest possible interleaved address)
	currentContent.startIt_ = nextNode->begin();
	currentContent.startIt_->setAddress(currentContent.lowerMask_);
	// end at the upper mask (= highest possible interleaved address)
	currentContent.endIt_ = nextNode->begin();
	currentContent.endIt_->setAddress(currentContent.upperMask_ + 1);

/*	currentContent.skipStack = false;
	if ((*currentContent.startIt_) == (*currentContent.endIt_)) {
		// there is only one relevant content in this node!
		// (i.e. if there is a subnode at the only contained address)
		// --> it can be skipped when returning from other stack contents
		currentAddressContent = *(*currentContent.startIt_);
		if (currentAddressContent.hasSubnode) {
			currentContent.skipStack = true;
			stepDown(currentAddressContent.subnode, currentAddressContent.address);
		}
	}*/
}


#endif /* SRC_ITERATORS_RANGEQUERYITERATOR_H_ */
