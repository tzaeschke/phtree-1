/*
 * RangeQueryStackContent.h
 *
 *  Created on: May 18, 2016
 *      Author: max
 */

#ifndef SRC_ITERATORS_RANGEQUERYSTACKCONTENT_H_
#define SRC_ITERATORS_RANGEQUERYSTACKCONTENT_H_

#include "nodes/Node.h"
#include "iterators/NodeIterator.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class RangeQueryStackContent {
public:
	bool fullyContained;
	bool lowerContained;
	bool upperContained;
	size_t lowerCompEqual;
	size_t lowerCompSmaller;
	size_t upperCompEqual;
	size_t upperCompSmaller;
	size_t lowerMask_;
	size_t upperMask_;
	size_t prefixLength_;
	const Node<DIM>* node_;
	// TODO make iterators lokal
	NodeIterator<DIM>* startIt_;
	NodeIterator<DIM>* endIt_;
};

#endif /* SRC_ITERATORS_RANGEQUERYSTACKCONTENT_H_ */
