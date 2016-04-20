/*
 * AHCIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef AHCITERATOR_H_
#define AHCITERATOR_H_

#include "nodes/AHC.h"
#include "iterators/NodeIterator.h"

class AHCIterator: public NodeIterator {
public:
	AHCIterator(AHC& node);
	AHCIterator(unsigned long address, AHC& node);
	virtual ~AHCIterator();

	void setAddress(size_t address) override;
	NodeIterator& operator++() override;
	NodeIterator operator++(int) override;
	NodeAddressContent operator*() override;

private:
	AHC* node_;
};

#endif /* AHCITERATOR_H_ */
