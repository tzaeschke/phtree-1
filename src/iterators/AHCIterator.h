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

template <unsigned int DIM>
class AHCIterator: public NodeIterator<DIM> {
public:
	AHCIterator(AHC<DIM>& node);
	AHCIterator(unsigned long address, AHC<DIM>& node);
	virtual ~AHCIterator();

	void setAddress(size_t address) override;
	NodeIterator<DIM>& operator++() override;
	NodeIterator<DIM> operator++(int) override;
	NodeAddressContent<DIM> operator*() override;

private:
	AHC<DIM>* node_;
};

#include <stdexcept>
#include <assert.h>
#include "nodes/Node.h"

template <unsigned int DIM>
AHCIterator<DIM>::AHCIterator(AHC<DIM>& node) : NodeIterator<DIM>() {
	node_ = &node;
	setAddress(0uL);
}

template <unsigned int DIM>
AHCIterator<DIM>::AHCIterator(unsigned long address, AHC<DIM>& node) : NodeIterator<DIM>(address) {
	node_ = &node;
	setAddress(address);
}

template <unsigned int DIM>
AHCIterator<DIM>::~AHCIterator() {}

template <unsigned int DIM>
void AHCIterator<DIM>::setAddress(size_t address) {
	if (address >= (1uL << DIM)) {
		this->address_ = (1uL << DIM);
	} else {
		// find first filled address if the given one is not filled
		for (this->address_ = address;
				!node_->contents_[this->address_].filled && address <=(1uL << DIM);
				this->address_++) {}
		if ((this->address_ == (1uL << DIM) - 1) && !node_->contents_[this->address_].filled)
			this->address_++;
	}
}

template <unsigned int DIM>
NodeIterator<DIM>& AHCIterator<DIM>::operator++() {
	// skip all unfilled fields until the highest address is reached
	for (this->address_++;
			this->address_ < (1uL << DIM) && !node_->contents_[this->address_].filled;
			this->address_++) {}
	if ((this->address_ == (1uL << DIM) - 1uL) && !node_->contents_[this->address_].filled)
		this->address_++;
	return *this;
}

template <unsigned int DIM>
NodeIterator<DIM> AHCIterator<DIM>::operator++(int) {
	 throw std::runtime_error("not implemented");
}

template <unsigned int DIM>
NodeAddressContent<DIM> AHCIterator<DIM>::operator*() {

	NodeAddressContent<DIM> content;
	if (!node_->contents_[this->address_].filled) {
		content.exists = false;
	} else {
		content.exists = true;
		content.address = this->address_;
		content.hasSubnode = node_->contents_[this->address_].hasSubnode;
		if (content.hasSubnode) {
			content.subnode = node_->contents_[this->address_].subnode;
		} else {
			content.suffix = &(node_->suffixes_[this->address_]);
			content.id = node_->contents_[this->address_].id;
		}
	}

	return content;
}

#endif /* AHCITERATOR_H_ */
