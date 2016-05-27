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

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class AHCIterator: public NodeIterator<DIM> {
public:
	AHCIterator(const AHC<DIM, PREF_BLOCKS>& node);
	AHCIterator(unsigned long address, const AHC<DIM, PREF_BLOCKS>& node);
	~AHCIterator();

	void setAddress(size_t address) override;
	NodeIterator<DIM>& operator++() override;
	NodeIterator<DIM> operator++(int) override;
	NodeAddressContent<DIM> operator*() override;

private:
	const AHC<DIM, PREF_BLOCKS>* node_;
};

#include <stdexcept>
#include <assert.h>
#include "nodes/Node.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHCIterator<DIM, PREF_BLOCKS>::AHCIterator(const AHC<DIM, PREF_BLOCKS>& node) : NodeIterator<DIM>(), node_(&node) {
	setAddress(0uL);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHCIterator<DIM, PREF_BLOCKS>::AHCIterator(unsigned long address, const AHC<DIM, PREF_BLOCKS>& node) : NodeIterator<DIM>(address), node_(&node) {
	setAddress(address);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHCIterator<DIM, PREF_BLOCKS>::~AHCIterator() {}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHCIterator<DIM, PREF_BLOCKS>::setAddress(size_t address) {
	if (address >= (1uL << DIM)) {
		this->address_ = (1uL << DIM);
	} else {
		// find first filled address if the given one is not filled
		bool filled = false;
		bool hasSub = false;
		uintptr_t ref = 0;
		for (this->address_ = address; this->address_ < (1uL << DIM); this->address_++) {
			node_->getRef(this->address_, &filled, &hasSub, &ref);
			if (filled) break;
		}

		if (this->address_ == (1 << DIM) - 1 && !filled) {
			this->address_ = 1 << DIM;
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>& AHCIterator<DIM, PREF_BLOCKS>::operator++() {
	// skip all unfilled fields until the highest address is reached
	setAddress(this->address_ + 1);
	return *this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM> AHCIterator<DIM, PREF_BLOCKS>::operator++(int) {
	 throw std::runtime_error("not implemented");
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeAddressContent<DIM> AHCIterator<DIM, PREF_BLOCKS>::operator*() {

	NodeAddressContent<DIM> content;
	node_->lookup(this->address_, content);
	content.address = this->address_;
	return content;
}

#endif /* AHCITERATOR_H_ */
