/*
 * RangeQueryIterator.cpp
 *
 *  Created on: Mar 16, 2016
 *      Author: max
 */

#include <assert.h>
#include "RangeQueryIterator.h"
#include "../util/MultiDimBitTool.h"
#include "../nodes/NodeAddressContent.h"

using namespace std;

// TODO also provide addresses per node that were followed during the lookup!
RangeQueryIterator::RangeQueryIterator(vector<Node*>* nodeStack, size_t dim, size_t bitLength, const Entry* lowerLeft, const Entry* upperRight) {
	assert (nodeStack->size() > 0 && "Should at least contain the root node");
	lowerLeftCorner_ = lowerLeft;
	upperRightCorner_ = upperRight;
	dim_ = dim;
	bitLength_ = bitLength;
	// TODO check of range is empty
	hasNext_ = true;

	currentPrefix_ = new vector<vector<bool>>(dim);
	for (size_t i = 0; i < currentPrefix_->size(); ++i) {
		currentPrefix_->at(i) = vector<bool>();
	}

	nodeStack_ = new stack<Node*>();
	lastAddressStack_ = new stack<unsigned long>();
	currentNode_ = nodeStack->at(0);
	currentIndex_ = 0;
	calculateRangeMasks();
	// TODO use masks
	currentHCAddress_ = 0;
	currentStartIterator_ = currentNode_->begin();
	currentStartIterator_->setAddress(currentHCAddress_);
	currentEndIterator_ = currentNode_->end();

	for (size_t i = 1; i < nodeStack->size(); ++i) {
		// do not push the current address of the root node
		stepDown(nodeStack->at(i));
	}
}

RangeQueryIterator::~RangeQueryIterator() {
	for (size_t i = 0; i < currentPrefix_->size(); i++) {
		currentPrefix_->at(i).clear();
	}
	currentPrefix_->clear();
	// TODO how to clear stacks
}

Entry RangeQueryIterator::next() {
	assert (hasNext());
	assert (currentPrefix_->size() == dim_);
	assert (currentPrefix_->at(0).size() == currentIndex_);

	// extract the next entry from a suffix
	// (potentially need to descend into subnodes)
	NodeAddressContent content = *(*currentStartIterator_);
	while (!isInMaskRange(content.address)) {
		++(*currentStartIterator_);
		content = *(*currentStartIterator_);
	}

	assert(content.exists);
	currentHCAddress_ = content.address;
	while (content.hasSubnode) {
		stepDown(content.subnode);
	}

	// found a valid suffix in the range
	Entry entry = MultiDimBitTool::createEntryFrom(currentPrefix_, currentHCAddress_, content.suffix, content.id);
	assert (entry.getDimensions() == dim_);
	assert (entry.getBitLength() == bitLength_);

	// set the iterator to the next element
	// (which is potentially in a higher node if the end was reached
	// and might not be the next element returned by the iterator as it can
	// be invalid according to the range mask)
	++(*currentStartIterator_);
	while (hasNext_ && (*currentStartIterator_) == (*currentEndIterator_)) {
		stepUp();
		if (hasNext_) {
			// ascended to the position from which the last node was handled -> go to next value
			++(*currentStartIterator_);
			// stop at the next value that is inside the range (otherwise iterate to end of mask)
			while ((*currentStartIterator_) != (*currentEndIterator_)) {
				content = *(*currentStartIterator_);
				if (isInMaskRange(content.address)) {
					break;
				}
				++(*currentStartIterator_);
			}
		}
	}

	return entry;
}

bool RangeQueryIterator::hasNext() {
	return hasNext_;
}

void RangeQueryIterator::calculateRangeMasks() {
	// reset masks
	currentLowerMask_ = 0;
	currentUpperMask_ = 0;

	// compare the lower left / upper right corner of the current node (including previous prefixes)
	// with the lower left / upper right corner of the range (i.e. only bit stings of same length)
	// for each dimension
	for (size_t d = 0; d < dim_; ++d)
	{
		// TODO compare center of current node to lower left / upper right!!!
		unsigned long prefixVal = MultiDimBitTool::bitsetToLong(&(currentPrefix_->at(d)));
		unsigned long nodeLowerLeft = prefixVal << (bitLength_ - currentIndex_);
		unsigned long nodeUpperRight = (prefixVal << (bitLength_ - currentIndex_));
		nodeUpperRight += ((1 << (bitLength_ - currentIndex_)) - 1);
		unsigned long currentLowerLeft = MultiDimBitTool::bitsetToLong(&(lowerLeftCorner_->values_.at(d)));
		unsigned long currentUpperRight = MultiDimBitTool::bitsetToLong(&(upperRightCorner_->values_.at(d)));
		if (currentLowerLeft > nodeLowerLeft) {
			// the lower left corner of the current node is outside of the range (in this dimension)
			currentLowerMask_ |= 1 << d;
		}

		if (currentUpperRight >= nodeUpperRight) {
			// the upper right corner of the current node is inside the range (in this dimension)
			currentUpperMask_ |= 1 << d;
		}
	}

	assert (currentLowerMask_ <= currentUpperMask_);
}

bool RangeQueryIterator::isInMaskRange(unsigned long hcAddress) {
	bool lowerMatch = (hcAddress | currentLowerMask_) == hcAddress;
	bool upperMatch = (hcAddress & currentUpperMask_) == hcAddress;
	return lowerMatch && upperMatch;
}

void RangeQueryIterator::stepUp() {
	if (nodeStack_->empty()) {
		hasNext_ = false;
	} else {
		// remove the prefix of the last node
		MultiDimBitTool::removeFirstBits(currentNode_->getPrefixLength() + 1, currentPrefix_);
		currentIndex_ -= (currentNode_->getPrefixLength() + 1);
		// restore the node and the HC address from the stack
		currentNode_ = nodeStack_->top();
		nodeStack_->pop();
		currentHCAddress_ = lastAddressStack_->top();
		lastAddressStack_->pop();
		// restore the iterators from the last HC address on the stack
		calculateRangeMasks();
		currentStartIterator_ = currentNode_->begin();
		currentStartIterator_->setAddress(currentHCAddress_);
		// TODO set lower and upper as addresses for the start and end iterator
		currentEndIterator_ = currentNode_->end();
	}
}

void RangeQueryIterator::stepDown(Node* nextNode) {
	// add the prefix of the next node to the current prefix
	MultiDimBitTool::pushValueToBack(currentPrefix_, currentHCAddress_);
	currentIndex_ += 1;

	MultiDimBitTool::pushBitsToBack(currentPrefix_, &(nextNode->prefix_));
	currentIndex_ += nextNode->getPrefixLength();
	// store the node and the current HC address on a stack
	lastAddressStack_->push(currentHCAddress_);
	nodeStack_->push(currentNode_);
	currentNode_ = nextNode;
	// create the iterators for the new node
	calculateRangeMasks();
	// TODO set lower and upper as addresses for the start and end iterator
	currentHCAddress_ = currentLowerMask_;
	currentStartIterator_ = currentNode_->begin();
	currentStartIterator_->setAddress(currentLowerMask_);
	currentEndIterator_ = currentNode_->begin();
	// the current upper mask is the last address potentially contained in the range
	currentEndIterator_->setAddress(currentUpperMask_ + 1);
}

