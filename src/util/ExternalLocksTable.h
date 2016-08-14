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

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class ExternalLocksTable {
public:

	ExternalLocksTable(size_t nThreads);
	virtual ~ExternalLocksTable() {}

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

	static const uintptr_t NIL = static_cast<uintptr_t>(NULL);
	static const uintptr_t FLAG_MASK = ~7;
	static const uintptr_t EXTRACT_COUNT_MASK = 7;

	static const size_t REFS_PER_THREAD = 2;
	// indicates that the thread is searching for the reference
	static const size_t STATUS_SEARCHING = -1;
	// indicates that the thread is searching for the reference
	// but found a thread with a lower index that is searching
	// for the same reference
	static const size_t STATUS_SEARCHING_SURRENDER = -2;
	// indicates that the reference is not valid any more
	static const size_t STATUS_INVALID = -3;

	size_t nThreads_;
	std::vector<std::atomic<size_t>> lockInUse_;
	std::vector<std::atomic<size_t>> lockIndex_;
	std::vector<std::atomic<uintptr_t>> refs_;
	std::vector<boost::shared_mutex> locks_; // TODO maybe use more locks than there are reference to reduce contention

	boost::shared_mutex* findLock(Node<DIM>* node, size_t threadIndex);
	boost::shared_mutex* useLock(Node<DIM>* node, size_t threadIndex);
	void freeLock(Node<DIM>* node, size_t threadIndex);

	inline void countDown(size_t refIndex);
	inline bool countUpIfMatch(size_t refIndex, uintptr_t ref);
};

using namespace std;

#include "nodes/Node.h"

template <unsigned int DIM>
ExternalLocksTable<DIM>::ExternalLocksTable(size_t nThreads) : nThreads_(nThreads),
	lockInUse_(REFS_PER_THREAD * nThreads), lockIndex_(REFS_PER_THREAD * nThreads),
	refs_(REFS_PER_THREAD * nThreads), locks_(REFS_PER_THREAD * nThreads) {
	for (unsigned i = 0; i < REFS_PER_THREAD * nThreads; ++i) {
		lockInUse_[i] = false;
		refs_[i] = NIL;
		lockIndex_[i] = STATUS_INVALID;
	}
}

template <unsigned int DIM>
boost::shared_mutex* ExternalLocksTable<DIM>::findLock(Node<DIM>* node, size_t threadIndex) {
	// finds a new lock by checking if the given reference (and thus also the lock) is
	// already held by another thread. If so it is reused and the usage counter is increased.
	// Otherwise a new lock is acquired.

	const uintptr_t ref = reinterpret_cast<uintptr_t>(node);
	assert ((ref & FLAG_MASK) == ref);
	size_t offset = REFS_PER_THREAD * threadIndex;
	assert (((refs_[offset] & FLAG_MASK) != ref) && ((refs_[offset + 1] & FLAG_MASK) != ref));
	// take one free slot of the two possible ones owned by the thread
	if (refs_[offset] != NIL) { offset += 1; }

	// mark the intent to find the given reference
	assert (lockIndex_[offset] == STATUS_INVALID);
	assert (refs_[offset] == NIL);

	size_t lockIndex, otherOffset;
	bool otherThreadHasRef = false;

	lockIndex_[offset] = STATUS_SEARCHING;
	// make reference active
	refs_[offset] = ref;

	// find another thread that has the same reference
	for (unsigned i = 0; i < nThreads_ && !otherThreadHasRef; ++i) {
		if (i == threadIndex) continue;
		for (unsigned j = 0; j < REFS_PER_THREAD && !otherThreadHasRef; ++j) {
			const size_t index = i * REFS_PER_THREAD + j;
			// flag the other threads reference so the other thread cannot remove it
			// (only if the reference is matching the desired one)
			if (countUpIfMatch(index, ref)) {
				otherOffset = index;
				if (threadIndex > i) {
					// I am the less important thread so surrender and spin until the other found the answer
					lockIndex_[offset] = STATUS_SEARCHING_SURRENDER;
				}

				do {
					lockIndex = lockIndex_[index];
				} while (lockIndex == STATUS_SEARCHING);

				assert (lockIndex != STATUS_INVALID && lockIndex != STATUS_SEARCHING);
				// found the correct lock index if the other did not surrender
				otherThreadHasRef = lockIndex != STATUS_SEARCHING_SURRENDER;

				if (!otherThreadHasRef) {
					countDown(otherOffset);
				}
			}
		}
	}

	if (otherThreadHasRef) {
		++lockInUse_[lockIndex];
		countDown(otherOffset);
	} else {
		// cannot reuse a lock so find a new one
		bool gotLock;
		lockIndex = offset;
		const size_t want = 1;
		do {
			lockIndex = (lockIndex + 1) % (REFS_PER_THREAD * nThreads_);
			size_t currentHope = 0;
			gotLock = atomic_compare_exchange_strong(&lockInUse_[lockIndex], &currentHope, want);
		} while (!gotLock);
	}

	lockIndex_[offset] = lockIndex;
	return &locks_[lockIndex];
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::countDown(size_t refIndex) {
	const uintptr_t ref = refs_[refIndex];
	assert (ref != NIL);
	assert ((ref & EXTRACT_COUNT_MASK) > 0);
	bool success;
	uintptr_t current, want;
	current = ref;
	do {
		want = current - 1;
		assert ((want & FLAG_MASK) == (ref & FLAG_MASK));
		success = atomic_compare_exchange_strong(&refs_[refIndex], &current, want);
	} while (!success);
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::countUpIfMatch(size_t refIndex, uintptr_t wantRef) {
	if (wantRef == NIL) return false;

	assert ((wantRef & EXTRACT_COUNT_MASK) == 0);
	bool success;
	uintptr_t current = wantRef;
	do {
		uintptr_t want = current + 1;
		success = atomic_compare_exchange_strong(&refs_[refIndex], &current, want);

		if (!success) {
			if ((current & FLAG_MASK) != wantRef) {
				// the reference at the given index did not match the desired reference
				return false;
			}

			unsigned int currentCount;
			do {
				// there are 3 bits for the read count so need to wait to increase the value until
				// the current readers are at most dec(6) = bin(110)
				currentCount = current & EXTRACT_COUNT_MASK;
				current = refs_[refIndex];
			} while (currentCount > 6);

			if ((current & FLAG_MASK) != wantRef) {
				// the reference at the given index did not match the desired reference
				return false;
			}
		}
	} while (!success);

	assert ((refs_[refIndex] & EXTRACT_COUNT_MASK) > 0);
	assert ((wantRef != NIL));
	return true;
}

template <unsigned int DIM>
boost::shared_mutex* ExternalLocksTable<DIM>::useLock(Node<DIM>* node, size_t threadIndex) {
	// check which of the two stored references to use
	// do not change the lock, the index, the state or the counter

	uintptr_t ref = reinterpret_cast<uintptr_t>(node);
	size_t offset = REFS_PER_THREAD * threadIndex;
	if ((refs_[offset] & FLAG_MASK) != ref) {
		assert ((refs_[offset + 1] & FLAG_MASK) == ref);
		offset += 1;
	}
	assert (refs_[offset] != NIL);

	const size_t lockIndex = lockIndex_[offset];
	assert (lockIndex != STATUS_INVALID && lockIndex != STATUS_SEARCHING && lockIndex != STATUS_SEARCHING_SURRENDER);
	assert (lockInUse_[lockIndex] > 0);
	return &locks_[lockIndex];
}

template <unsigned int DIM>
void ExternalLocksTable<DIM>::freeLock(Node<DIM>* node, size_t threadIndex) {
	// frees the reference of the given node
	// that is it (1) waits until the proper reference is not flagged any more,
	// (2) sets the status to invalid and (3) reduces the usage counter

	const uintptr_t ref = reinterpret_cast<uintptr_t>(node);
	size_t offset = REFS_PER_THREAD * threadIndex;
	if ((refs_[offset] & FLAG_MASK) != ref) {
		assert ((refs_[offset + 1] & FLAG_MASK) == ref);
		offset += 1;
	}
	assert (refs_[offset] != NIL);

	const size_t prevLockIndex = lockIndex_[offset];
	assert (prevLockIndex != STATUS_INVALID && prevLockIndex != STATUS_SEARCHING && prevLockIndex != STATUS_SEARCHING_SURRENDER);
	assert (lockInUse_[prevLockIndex] > 0);
	bool flagged;
	do {
		uintptr_t currentRef = ref;
		flagged = !atomic_compare_exchange_strong(&refs_[offset], &currentRef, NIL);
		assert ((currentRef & FLAG_MASK) == ref);
	} while (flagged);

	lockIndex_[offset] = STATUS_INVALID;
	--lockInUse_[prevLockIndex];
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::assertNoLocks(size_t threadIndex) {
	size_t offset = REFS_PER_THREAD * threadIndex;

	for (unsigned j = 0; j < REFS_PER_THREAD; ++j) {
		assert (lockIndex_[offset + j] == STATUS_INVALID);
		assert (refs_[offset + j] == NIL);
	}

	return true;
}

template <unsigned int DIM>
bool ExternalLocksTable<DIM>::assertNoLocks() {

	// check that no thread has a reference to a lock
	for (unsigned i = 0; i < nThreads_; ++i) {
		assertNoLocks(i);
	}

	// check that all locks are unused
	for (unsigned i = 0; i < nThreads_; ++i) {
		for (unsigned j = 0; j < REFS_PER_THREAD; ++j) {
			assert (lockInUse_[i * REFS_PER_THREAD + j] == 0);
		}
	}

	return true;
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

#endif /* SRC_UTIL_EXTERNALLOCKSTABLE_H_ */
