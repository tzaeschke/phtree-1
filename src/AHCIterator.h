/*
 * AHCIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef AHCITERATOR_H_
#define AHCITERATOR_H_

#include "AHC.h"
#include "NodeIterator.h"

class AHCIterator: public NodeIterator {
public:
	AHCIterator(AHC& node);
	AHCIterator(long address, AHC& node);
	virtual ~AHCIterator();

	NodeIterator& operator++() override;
	NodeIterator operator++(int) override;
	NodeAddressContent& operator*() override;

private:
	AHC* node_;
};

#endif /* AHCITERATOR_H_ */
