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

template <unsigned int DIM>
struct NodeAddressContent;

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
class LHCIterator : public NodeIterator<DIM> {
public:
	explicit LHCIterator(const LHC<DIM, PREF_BLOCKS, N>& node);
	LHCIterator(unsigned long address, const LHC<DIM, PREF_BLOCKS, N>& node);
	~LHCIterator();

	void setToBegin() override;
	void setAddress(size_t address) override;
	NodeIterator<DIM>& operator++() override;
	NodeIterator<DIM> operator++(int) override;
	NodeAddressContent<DIM> operator*() const override;

private:
	const LHC<DIM, PREF_BLOCKS, N>* node_;
	unsigned int currentIndex;
};

#include <assert.h>
#include "iterators/LHCIterator.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
LHCIterator<DIM, PREF_BLOCKS, N>::LHCIterator(const LHC<DIM, PREF_BLOCKS, N>& node) : NodeIterator<DIM>(), node_(&node), currentIndex(0) {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
LHCIterator<DIM, PREF_BLOCKS, N>::LHCIterator(unsigned long address, const LHC<DIM, PREF_BLOCKS, N>& node) : NodeIterator<DIM>(address), node_(&node) {
	setAddress(address);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
LHCIterator<DIM, PREF_BLOCKS, N>::~LHCIterator() { }

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHCIterator<DIM, PREF_BLOCKS, N>::setAddress(size_t address) {
	// find first filled address if the given one is not filled

	bool exists = false;
	bool hasSub = false;
	bool directlyStored = false;
	node_->lookupAddress(address, &exists, &currentIndex, &hasSub, &directlyStored);
	if (exists) {
		// found address so set it
		this-> address_ = address;
	} else if (currentIndex > node_->m - 1) {
		// did not find the address and it is not in the range
		this->address_ = 1 << DIM;
	} else {
		// did not find the address but it is in the range
		node_->lookupIndex(currentIndex, &(this->address_), &hasSub, &directlyStored);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHCIterator<DIM, PREF_BLOCKS, N>::setToBegin() {
	this->currentIndex = 0;
	bool hasSub;
	bool directlyStored;
	// TODO address lookup needed or done before comparison anyway?
	node_->lookupIndex(0u, &(this->address_), &hasSub, &directlyStored);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>& LHCIterator<DIM, PREF_BLOCKS, N>::operator++() {
	++currentIndex;
	if (currentIndex >= node_->m) {
		currentIndex = node_->m - 1;
		this->address_ = 1 << DIM;
	} else {
		bool hasSub = false;
		bool directlyStored = false;
		node_->lookupIndex(currentIndex, &(this->address_), &hasSub, &directlyStored);
	}

	return *this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM> LHCIterator<DIM, PREF_BLOCKS, N>::operator++(int) {
	throw "++ i not implemented";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeAddressContent<DIM> LHCIterator<DIM, PREF_BLOCKS, N>::operator*() const {

	NodeAddressContent<DIM> content;
	content.exists = true;
	bool isPointer, isSuffix;
	node_->lookupIndex(currentIndex, &content.address, &isPointer, &isSuffix);
	node_->fillLookupContent(content,
			isPointer, isSuffix,
			node_->references_[currentIndex],
			this->resolveSuffixIndexToPointer_);
	return content;
}

#endif /* LHCITERATOR_H_ */
