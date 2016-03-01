/*
 * LHCIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef LHCITERATOR_H_
#define LHCITERATOR_H_

#include <map>
#include "Node.h"
#include "LHC.h"
#include "NodeIterator.h"

class LHCIterator : public NodeIterator {
public:
	LHCIterator(LHC& node);
	LHCIterator(long address, LHC& node);
	virtual ~LHCIterator();

	NodeIterator& operator++();
	NodeIterator operator++(int);
	bool operator==(const NodeIterator& rhs);
	bool operator!=(const NodeIterator& rhs);
	NodeAddressContent& operator*();

private:
	LHC* node_;
	map<long,NodeAddressContent*>::iterator contentMapIt_;
};

#endif /* LHCITERATOR_H_ */
