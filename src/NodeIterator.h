/*
 * NodeIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef NODEITERATOR_H_
#define NODEITERATOR_H_

#include <iterator>
#include "Node.h"
#include "NodeAddressContent.h"

class NodeIterator : public iterator<input_iterator_tag, NodeAddressContent> {
public:
	NodeIterator();
	NodeIterator(long address);
	virtual ~NodeIterator();

	virtual NodeIterator& operator++();
	virtual NodeIterator operator++(int);
	virtual bool operator==(const NodeIterator& rhs);
	virtual bool operator!=(const NodeIterator& rhs);
	virtual NodeAddressContent& operator*();

protected:
	long address_;
};

#endif /* NODEITERATOR_H_ */
