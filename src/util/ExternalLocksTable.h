/*
 * ExternalLocksTable.h
 *
 *  Created on: Aug 12, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_EXTERNALLOCKSTABLE_H_
#define SRC_UTIL_EXTERNALLOCKSTABLE_H_

#include <atomic>
#include <boost/thread/shared_mutex.hpp>
#include "util/ExternalFixedSizeLockTable.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class ExternalLocksTable {
public:

	ExternalLocksTable(size_t nThreads);
	virtual ~ExternalLocksTable();

	bool writeLock(Node<DIM>* node, size_t threadIndex);
	void writeUnlock(Node<DIM>* node, size_t threadIndex);
	void downgradeWriterToReader(Node<DIM>* node, size_t threadIndex);
	void readLockBlocking(Node<DIM>* node, size_t threadIndex);
	bool readLock(Node<DIM>* node, size_t threadIndex);
	void readUnlock(Node<DIM>* node, size_t threadIndex);
	bool tryWriteLockWithoutRead(Node<DIM>* node, size_t threadIndex);

	bool assertNoLocks(size_t threadIndex);
	bool assertNoLocks();
private:

	static const size_t FREE_LSB_BITS_IN_REF = 3;

	size_t nThreads_;
	size_t nMsbBitsShift_;
	std::vector<ExternalFixedSizeLocksTable*> tables_; // TODO make local

	boost::shared_mutex* findLock(Node<DIM>* node, size_t threadIndex);
	boost::shared_mutex* useLock(Node<DIM>* node, size_t threadIndex);
	void freeLock(Node<DIM>* node, size_t threadIndex);
	inline size_t refBitShift(uintptr_t& ref) const;
};

template <unsigned int DIM>
ExternalLocksTable<DIM>::ExternalLocksTable(size_t nThreads) : nThreads_(nThreads), nMsbBitsShift_(), tables_() {
	assert (nThreads > 0);
	nMsbBitsShift_ = sizeof (nThreads) * 8 - __builtin_clzl(nThreads);
	nMsbBitsShift_ = (nMsbBitsShift_ < FREE_LSB_BITS_IN_REF)? 0 : nMsbBitsShift_ - FREE_LSB_BITS_IN_REF;

	// need tables according to MSB bits shifted, e.g.:
	// assumes that the 3 least significant bits are always free
	// [MSB (3) | ref (58) | LSB (3)] => 2^3 tables to map all MSB bit combinations
	const size_t requiredTables = 1 << nMsbBitsShift_;

	tables_.reserve(requiredTables);
	for (unsigned i = 0; i < requiredTables; ++i) {
		//tables_.emplace_back(nThreads);
		tables_.push_back(new ExternalFixedSizeLocksTable(nThreads));
	}
}

template <unsigned int DIM>
ExternalLocksTable<DIM>::~ExternalLocksTable() {
	for (unsigned i = 0; i < tables_.size(); ++i) {
		delete tables_[i];
	}
}

template <unsigned int DIM>
size_t ExternalLocksTable<DIM>::refBitShift(uintptr_t& ref) const {
	if (nMsbBitsShift_ == 0) {
		return 0;
	} else {
		const size_t upperBits = ref >> (sizeof (uintptr_t) * 8 - nMsbBitsShift_);
		assert (tables_.size() > upperBits);
		ref = ref << nMsbBitsShift_;
		// [ref (64 - n - 3) |  (n) MSB free bits | (3) LSB free bits]
		return upperBits;
	}
}

template <unsigned int DIM>
boost::shared_mutex* ExternalLocksTable<DIM>::findLock(Node<DIM>* node, size_t threadIndex) {
	uintptr_t ref = reinterpret_cast<uintptr_t>(node);
	return tables_[refBitShift(ref)]->findLock(ref, threadIndex);
}

template <unsigned int DIM>
boost::shared_mutex* ExternalLocksTable<DIM>::useLock(Node<DIM>* node, size_t threadIndex) {
	uintptr_t ref = reinterpret_cast<uintptr_t>(node);
	return tables_[refBitShift(ref)]->useLock(ref, threadIndex);
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::freeLock(Node<DIM>* node, size_t threadIndex) {
	uintptr_t ref = reinterpret_cast<uintptr_t>(node);
	return tables_[refBitShift(ref)]->freeLock(ref, threadIndex);
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::writeLock(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = useLock(node, threadIndex);
	bool success = false;
	if (m->try_unlock_shared_and_lock_upgrade()) {
		// the thread now holds the only lock with upgrade privileges on the node
		m->unlock_upgrade_and_lock();
		success = true;
	}

	return success;
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::writeUnlock(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = useLock(node, threadIndex);
	m->unlock();
	freeLock(node, threadIndex);
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::downgradeWriterToReader(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = useLock(node, threadIndex);
	m->unlock_and_lock_shared();
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::readLockBlocking(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = findLock(node, threadIndex);
	m->lock_shared();
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::readLock(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = findLock(node, threadIndex);
	bool success = m->try_lock_shared();
	if (!success) {
		freeLock(node, threadIndex);
	}

	return success;
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::readUnlock(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = useLock(node, threadIndex);
	m->unlock_shared();
	freeLock(node, threadIndex);
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::tryWriteLockWithoutRead(Node<DIM>* node, size_t threadIndex) {
	boost::shared_mutex* m = findLock(node, threadIndex);
	bool success = m->try_lock();
	if (!success) {
		freeLock(node, threadIndex);
	}

	return success;
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::assertNoLocks(size_t threadIndex) {
	for (unsigned i = 0; i < tables_.size(); ++i) {
		tables_[i]->assertNoLocks(threadIndex);
	}

	return true;
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::assertNoLocks() {
	for (unsigned i = 0; i < tables_.size(); ++i) {
		tables_[i]->assertNoLocks();
	}

	return true;
}

#endif /* SRC_UTIL_EXTERNALLOCKSTABLE_H_ */
