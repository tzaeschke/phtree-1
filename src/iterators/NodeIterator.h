/*
 * NodeIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef NODEITERATOR_H_
#define NODEITERATOR_H_

#include <iterator>
#include "../nodes/NodeAddressContent.h"

class NodeIterator : public std::iterator<std::input_iterator_tag, NodeAddressContent> {
public:
	NodeIterator();
	NodeIterator(long address);
	virtual ~NodeIterator();
	bool operator==(const NodeIterator& rhs);
	bool operator!=(const NodeIterator& rhs);

	virtual void setAddress(size_t address);
	virtual NodeIterator& operator++();
	virtual NodeIterator operator++(int);
	virtual NodeAddressContent operator*();

protected:
	long address_;
	bool reachedEnd_;
};

#endif /* NODEITERATOR_H_ */
