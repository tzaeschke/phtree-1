/*
 * LHCIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef LHCITERATOR_H_
#define LHCITERATOR_H_

#include <map>
#include "NodeIterator.h"
#include "../nodes/LHC.h"

struct NodeAddressContent;

class LHCIterator : public NodeIterator {
public:
	LHCIterator(LHC& node);
	LHCIterator(long address, LHC& node);
	virtual ~LHCIterator();

	void setAddress(size_t address) override;
	NodeIterator& operator++() override;
	NodeIterator operator++(int) override;
	NodeAddressContent operator*() override;

private:
	LHC* node_;
	std::map<long,NodeAddressContent*>::iterator contentMapIt_;
};

#endif /* LHCITERATOR_H_ */
