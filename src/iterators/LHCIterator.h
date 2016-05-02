/*
 * LHCIterator.h
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef LHCITERATOR_H_
#define LHCITERATOR_H_

#include <map>
#include "iterators/NodeIterator.h"
#include "nodes/LHC.h"
#include "nodes/LHCAddressContent.h"

template <unsigned int DIM>
struct NodeAddressContent;

template <unsigned int DIM>
class LHCIterator : public NodeIterator<DIM> {
public:
	LHCIterator(LHC<DIM>& node);
	LHCIterator(unsigned long address, LHC<DIM>& node);
	virtual ~LHCIterator();

	void setAddress(size_t address) override;
	NodeIterator<DIM>& operator++() override;
	NodeIterator<DIM> operator++(int) override;
	NodeAddressContent<DIM> operator*() override;

private:
	LHC<DIM>* node_;
	typename std::map<unsigned long,LHCAddressContent<DIM>>::iterator contentMapIt_;
};

#include <assert.h>
#include "iterators/LHCIterator.h"

template <unsigned int DIM>
LHCIterator<DIM>::LHCIterator(LHC<DIM>& node) : NodeIterator<DIM>() {
	node_ = &node;
	setAddress(0);
}

template <unsigned int DIM>
LHCIterator<DIM>::LHCIterator(unsigned long address, LHC<DIM>& node) : NodeIterator<DIM>(address) {
	node_ = &node;
	setAddress(address);
}

template <unsigned int DIM>
LHCIterator<DIM>::~LHCIterator() { }

template <unsigned int DIM>
void LHCIterator<DIM>::setAddress(size_t address) {
	// find first filled address if the given one is not filled
	// TODO implement without starting at the front
	if (address >= DIM) {
		this->address_ = 1uL << DIM;
	} else {
		contentMapIt_ = node_->sortedContents_.begin();
		for (this->address_ = contentMapIt_->first;
				this->address_ < address && contentMapIt_ != node_->sortedContents_.end();
				++contentMapIt_) {
			this->address_ = contentMapIt_->first;
		}
	}
}

template <unsigned int DIM>
NodeIterator<DIM>& LHCIterator<DIM>::operator++() {

	if (++contentMapIt_ != node_->sortedContents_.end()) {
		this->address_ = contentMapIt_->first;
	} else {
		--contentMapIt_;
		this->reachedEnd_ = true;
		this->address_ = 1uL << DIM;
	}
	return *this;
}

template <unsigned int DIM>
NodeIterator<DIM> LHCIterator<DIM>::operator++(int i) {
	throw "++ i not implemented";
}

template <unsigned int DIM>
NodeAddressContent<DIM> LHCIterator<DIM>::operator*() {
	assert (contentMapIt_->first == this->address_);
	NodeAddressContent<DIM> content;
	content.exists = true;
	content.address = contentMapIt_->first;
	content.hasSubnode = contentMapIt_->second.hasSubnode;
	if (!contentMapIt_->second.hasSubnode) {
		content.suffix = &(contentMapIt_->second.suffix);
		content.id = contentMapIt_->second.id;
	} else {
		content.subnode = contentMapIt_->second.subnode;
	}

	return content;
}

#endif /* LHCITERATOR_H_ */
