/*
 * EntryBufferPool.h
 *
 *  Created on: Jul 13, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_ENTRYBUFFERPOOL_H_
#define SRC_UTIL_ENTRYBUFFERPOOL_H_

template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer;

template <unsigned int DIM, unsigned int WIDTH>
class EntryBufferPool {
public:
	EntryBufferPool();
	~EntryBufferPool() {};

	EntryBuffer<DIM, WIDTH>* allocate();
	void deallocate(EntryBuffer<DIM, WIDTH>* buffer);

	void reset();
	size_t prepareFullDeallocate();
	EntryBuffer<DIM,WIDTH>* get(size_t index);

	static const size_t capacity_ = 1000;
private:

	size_t headIndex_;
	size_t nInitialized_;
	size_t nFree_;
	EntryBuffer<DIM, WIDTH> pool_[capacity_];
};

#include <assert.h>
#include "util/EntryBuffer.h"

template <unsigned int DIM, unsigned int WIDTH>
EntryBufferPool<DIM, WIDTH>::EntryBufferPool() : headIndex_(0), nInitialized_(0), nFree_(capacity_), pool_() {
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBufferPool<DIM, WIDTH>::reset() {
	headIndex_ = 0;
	nInitialized_ = 0;
	nFree_ = capacity_;
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBufferPool<DIM, WIDTH>::prepareFullDeallocate() {
	assert (headIndex_ == (-1u) || headIndex_ < capacity_);

	// only disables the current items in the free list by flagging their index
	// attention: this destroys the free list so reset() should be called afterwards!
	size_t nextIndex = headIndex_;
	headIndex_ = -1u;
	while (nextIndex != (-1u) && nextIndex < nInitialized_) {
		const size_t nextIndexTmp = pool_[nextIndex].nextIndex_;
		pool_[nextIndex].nextIndex_ = -1u;
		nextIndex = nextIndexTmp;
	}

	return nInitialized_;
}

template <unsigned int DIM, unsigned int WIDTH>
EntryBuffer<DIM,WIDTH>* EntryBufferPool<DIM, WIDTH>::get(size_t index) {
	assert (index < capacity_);
	if (pool_[index].nextIndex_ == (-1u)) {
		return NULL;
	}

	return &pool_[index];
}

template <unsigned int DIM, unsigned int WIDTH>
EntryBuffer<DIM, WIDTH>* EntryBufferPool<DIM, WIDTH>::allocate() {

	if (nInitialized_ < capacity_) {
		// not all fields were initialized so add the next entry to the free list
		pool_[nInitialized_].nextIndex_ = nInitialized_ + 1;
		++nInitialized_;
	}

	EntryBuffer<DIM,WIDTH>* alloc = NULL;
	if (nFree_ > 0) {
		--nFree_;
		alloc = &pool_[headIndex_];
		if (nFree_ > 0) {
			headIndex_ = pool_[headIndex_].nextIndex_;
		} else {
			headIndex_ = -1u;
		}

		alloc->nextIndex_ = 0;
	}

	return alloc;
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBufferPool<DIM, WIDTH>::deallocate(EntryBuffer<DIM, WIDTH>* buffer) {
	const size_t diff = (size_t)buffer - (size_t)pool_;
	const size_t slotSize = sizeof (EntryBuffer<DIM, WIDTH>);
	assert (diff % slotSize == 0);
	const size_t index = diff / slotSize;
	assert ((size_t)buffer >= (size_t)pool_ && index < capacity_);

	if (headIndex_ != (-1u)) {
		buffer->nextIndex_ = headIndex_;
	}

	headIndex_ = index;
	++nFree_;
}

#endif /* SRC_UTIL_ENTRYBUFFERPOOL_H_ */
