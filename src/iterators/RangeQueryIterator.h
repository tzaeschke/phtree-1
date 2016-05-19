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
			const Entry<DIM, WIDTH>* lowerLeft,
			const Entry<DIM, WIDTH>* upperRight);
	virtual ~RangeQueryIterator();

	Entry<DIM, WIDTH> next();
	bool hasNext() const;

private:
	bool hasNext_;
	size_t currentIndex_;

	std::stack<RangeQueryStackContent<DIM>> stack_;
	RangeQueryStackContent<DIM> currentContent;
	unsigned long currentValue[1 + (DIM * WIDTH - 1) / (sizeof (unsigned long) * 8)];

	const Entry<DIM, WIDTH>* lowerLeftCorner_;
	const Entry<DIM, WIDTH>* upperRightCorner_;


	void stepUp();
	void stepDown(const Node<DIM>* nextNode, unsigned long hcAddress);
	inline bool isInMaskRange(unsigned long hcAddress) const;
	void createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength);
};

#include <assert.h>
#include "nodes/NodeAddressContent.h"

using namespace std;

// TODO also provide addresses per node that were followed during the lookup!
template <unsigned int DIM, unsigned int WIDTH>
RangeQueryIterator<DIM, WIDTH>::RangeQueryIterator(vector<pair<unsigned long, const Node<DIM>*>>* visitedNodes,
		const Entry<DIM, WIDTH>* lowerLeft, const Entry<DIM, WIDTH>* upperRight) : hasNext_(true),
		currentIndex_(0), stack_(),
		currentValue(), lowerLeftCorner_(lowerLeft),
		upperRightCorner_(upperRight) {

	assert (!visitedNodes->empty());
	if (visitedNodes->empty()) {
		hasNext_ = false;
	} else {
		// the first node has to bee the root node which does not have a prefix!
		createCurrentContent(visitedNodes->at(0).second, 0);
		for (unsigned int i = 1; i < visitedNodes->size(); ++i) {
			const pair<unsigned long, const Node<DIM>*> nextNode = (*visitedNodes)[i];
			stepDown(nextNode.second, nextNode.first);
		}
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

	// extract the next entry from a suffix
	// (potentially need to descend into subnodes)
	NodeAddressContent<DIM> content = *(*currentContent.startIt_);
	assert(content.exists && isInMaskRange(content.address));
	while (content.hasSubnode) {
		assert(content.exists && isInMaskRange(content.address));
		stepDown(content.subnode, content.address);
		content = *(*currentContent.startIt_);
	}

	// found a valid suffix in the range
	Entry<DIM, WIDTH> entry(currentValue, content.id);
	// copy the suffix into the entry
	MultiDimBitset<DIM>::pushBackValue(content.address, entry.values_, DIM * (WIDTH - currentIndex_ - 1));
	const size_t suffixBits = DIM * (WIDTH - (currentIndex_ + 1));
	MultiDimBitset<DIM>::pushBackBitset(content.suffixStartBlock, suffixBits, entry.values_, 0);

	// iterate to the next valid address (might be in a higher node)
	do {
		++(*currentContent.startIt_);
		if ((*currentContent.startIt_) == (*currentContent.endIt_) && hasNext_) {
			// reached the end of the range in the node so ascend to a higher node
			stepUp();
		}

		content = *(*currentContent.startIt_);
		// go to next valid entry in the node range
		while ((*currentContent.startIt_) != (*currentContent.endIt_) && isInMaskRange(content.address)) {
			++(*currentContent.startIt_);
			content = *(*currentContent.startIt_);
		}
	} while (hasNext_ && (*currentContent.startIt_) == (*currentContent.endIt_));

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

	bool lowerMatch = (hcAddress | currentContent.lowerMask_) == hcAddress;
	bool upperMatch = (hcAddress & currentContent.upperMask_) == hcAddress;
	return lowerMatch && upperMatch;
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::stepUp() {
	if (stack_.empty()) {
		hasNext_ = false;
	} else {
		// remove the prefix of the last node
		const size_t prefixLength = currentContent.prefixLength_;
		MultiDimBitset<DIM>::removeHighestBits(currentValue, currentIndex_ * DIM, (1 + prefixLength) * DIM);
		currentIndex_ -= (prefixLength + 1);

		// clear memory
		delete currentContent.startIt_;
		delete currentContent.endIt_;

		// restore the node and the HC address from the stack
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
	currentIndex_ += prefixLength;
	MultiDimBitset<DIM>::pushBackBitset(nextNode->getFixPrefixStartBlock(), prefixLength * DIM,
			currentValue, (WIDTH - currentIndex_) * DIM);

	stack_.push(currentContent);
	createCurrentContent(nextNode, prefixLength);
}

template <unsigned int DIM, unsigned int WIDTH>
void RangeQueryIterator<DIM, WIDTH>::createCurrentContent(const Node<DIM>* nextNode, size_t prefixLength) {
	assert (nextNode->getPrefixLength() == prefixLength);
	currentContent.node_ = nextNode;
	currentContent.prefixLength_ = prefixLength;

	// calculate the range masks for the next node
	const unsigned int ignoreNLowestBits = DIM * (WIDTH - currentIndex_);
	// <isSmaller bitset, isEqual bitset>
	pair<unsigned long, unsigned long> lowerComp = MultiDimBitset<DIM>::
			compareSmallerEqual(lowerLeftCorner_->values_, currentValue, DIM * WIDTH, ignoreNLowestBits);
	pair<unsigned long, unsigned long> upperComp = MultiDimBitset<DIM>::
			compareSmallerEqual(upperRightCorner_->values_, currentValue, DIM * WIDTH, ignoreNLowestBits);

	const unsigned int dimMask = (1u << DIM) - 1u;
	currentContent.lowerMask_ = lowerComp.first | ((~lowerComp.second) & dimMask);
	currentContent.upperMask_ = ((~upperComp.first) & dimMask) | upperComp.second;
	currentContent.currentHcAddress_ = currentContent.lowerMask_;
	currentContent.startIt_ = nextNode->begin();
	currentContent.startIt_->setAddress(currentContent.lowerMask_);
	currentContent.endIt_ = nextNode->begin();
	currentContent.endIt_->setAddress(currentContent.upperMask_);
}


#endif /* SRC_ITERATORS_RANGEQUERYITERATOR_H_ */
