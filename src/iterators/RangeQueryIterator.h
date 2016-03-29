/*
 * RangeQueryIterator.h
 *
 *  Created on: Mar 16, 2016
 *      Author: max
 */

#ifndef SRC_ITERATORS_RANGEQUERYITERATOR_H_
#define SRC_ITERATORS_RANGEQUERYITERATOR_H_

#include <stack>
#include "../nodes/Node.h"
#include "NodeIterator.h"
#include "../Entry.h"

class Node;

class RangeQueryIterator {
public:
	RangeQueryIterator(std::vector<Node*>* nodeStack, size_t dim, size_t bitLength, Entry* lowerLeft, Entry* upperRight);
	virtual ~RangeQueryIterator();

	Entry next();
	bool hasNext();

private:
	size_t dim_;
	size_t currentIndex_;
	size_t bitLength_;
	unsigned long currentHCAddress_;
	std::stack<Node*>* nodeStack_;
	std::stack<unsigned long>* lastAddressStack_;
	std::vector<std::vector<bool>>* currentPrefix_;
	Node* currentNode_;
	NodeIterator* currentStartIterator_;
	NodeIterator* currentEndIterator_;
	size_t currentLowerMask_;
	size_t currentUpperMask_;
	Entry* upperRightCorner_;
	Entry* lowerLeftCorner_;
	bool hasNext_;


	void calculateRangeMasks();
	void stepUp();
	void stepDown(Node* nextNode);
	bool isInMaskRange(unsigned long hcAddress);
};

#endif /* SRC_ITERATORS_RANGEQUERYITERATOR_H_ */