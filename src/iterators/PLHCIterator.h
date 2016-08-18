/*
 * PLHCIterator.h
 *
 *  Created on: Aug 17, 2016
 *      Author: max
 */

#ifndef SRC_ITERATORS_PLHCITERATOR_H_
#define SRC_ITERATORS_PLHCITERATOR_H_

#include <map>
#include "iterators/NodeIterator.h"
#include "nodes/PLHC.h"

template <unsigned int DIM>
struct NodeAddressContent;

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
class PLHCIterator : public NodeIterator<DIM> {
public:
	explicit PLHCIterator(const PLHC<DIM, PREF_BLOCKS, N>& node);
	~PLHCIterator();

	void setToBegin() override;
	void setAddress(size_t address) override;
	NodeIterator<DIM>& operator++() override;
	NodeIterator<DIM> operator++(int) override;
	NodeAddressContent<DIM> operator*() const override;

private:
	size_t key_[N];
	uintptr_t indexValue_[N];
	const PLHC<DIM, PREF_BLOCKS, N>* node_;
	bool nextFromMerged;
	unsigned int currentSortedIndex;
	unsigned int currentMergedIndex;

	void sortUnsortedSequence();
	inline bool hasNext() const;
	inline void setNextLocation();
};

#include <assert.h>
#include "iterators/PLHCIterator.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
PLHCIterator<DIM, PREF_BLOCKS, N>::PLHCIterator(const PLHC<DIM, PREF_BLOCKS, N>& node) : NodeIterator<DIM>(),
	key_(), indexValue_(), node_(&node), nextFromMerged(false), currentSortedIndex(0), currentMergedIndex(0) {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
PLHCIterator<DIM, PREF_BLOCKS, N>::~PLHCIterator() { }

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHCIterator<DIM, PREF_BLOCKS, N>::setAddress(size_t address) {
	throw "unsupported";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHCIterator<DIM, PREF_BLOCKS, N>::setToBegin() {
	sortUnsortedSequence();
	currentMergedIndex = 0;
	currentSortedIndex = 0;
	assert (hasNext());
	setNextLocation();
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
inline bool PLHCIterator<DIM, PREF_BLOCKS, N>::hasNext() const {
	return (currentMergedIndex < node_->mUnsorted) || (currentSortedIndex < node_->mSorted);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
inline void PLHCIterator<DIM, PREF_BLOCKS, N>::setNextLocation() {
	if (currentMergedIndex == node_->mUnsorted) {
		nextFromMerged = false;
	} else if (currentSortedIndex == node_->mSorted) {
		nextFromMerged = true;
	} else {
		unsigned long sortedKey = 0;
		node_->lookupIndexSorted(currentSortedIndex, &sortedKey);
		assert (sortedKey != key_[currentMergedIndex]);
		nextFromMerged = key_[currentMergedIndex] < sortedKey;
	}

	if (nextFromMerged) {
		this->address_ = key_[currentMergedIndex];
	} else {
		node_->lookupIndexSorted(currentSortedIndex, &(this->address_));
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHCIterator<DIM, PREF_BLOCKS, N>::sortUnsortedSequence() {
	// perform insertion sort
	const size_t n = node_->mUnsorted;
	if (n > 0) {
		key_[0] = node_->unorderedAddresses_[0];
		indexValue_[0] = 0;

		size_t i, j;
		for (i = 1; i < n; ++i) {
			size_t tmp = node_->unorderedAddresses_[i];
			for (j = i; j >= 1 && tmp < key_[j - 1]; j--) {
				key_[j] = key_[j - 1];
				indexValue_[j] = indexValue_[j - 1];
			}
			key_[j] = tmp;
			indexValue_[j] = i;
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>& PLHCIterator<DIM, PREF_BLOCKS, N>::operator++() {

	if (nextFromMerged) {
		++currentMergedIndex;
	} else {
		++currentSortedIndex;
	}

	if (hasNext()) {
		setNextLocation();
	} else {
		this->address_ = 1 << DIM;
	}

	return *this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM> PLHCIterator<DIM, PREF_BLOCKS, N>::operator++(int) {
	throw "++ i not implemented";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeAddressContent<DIM> PLHCIterator<DIM, PREF_BLOCKS, N>::operator*() const {
	NodeAddressContent<DIM> content;
	content.exists = true;
	content.address = this->address_;
	uintptr_t ref;

	if (nextFromMerged) {
		ref = node_->unorderedReferences_[indexValue_[currentMergedIndex]];
	} else {
		ref = node_->orderedReferences_[currentSortedIndex];
	}

	node_->fillLookupContent(content, ref,
			this->resolveSuffixIndexToPointer_);
	return content;
}

#endif /* SRC_ITERATORS_PLHCITERATOR_H_ */
