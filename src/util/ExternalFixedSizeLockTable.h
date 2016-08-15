/*
 * ExternalFixedSizeLockTable.h
 *
 *  Created on: Aug 14, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_EXTERNALFIXEDSIZELOCKTABLE_H_
#define SRC_UTIL_EXTERNALFIXEDSIZELOCKTABLE_H_

#include <atomic>
#include <boost/thread/shared_mutex.hpp>

class ExternalFixedSizeLocksTable {
public:

	ExternalFixedSizeLocksTable();
	ExternalFixedSizeLocksTable(size_t nThreads);
	virtual ~ExternalFixedSizeLocksTable() {}

	boost::shared_mutex* findLock(uintptr_t ref, size_t threadIndex);
	boost::shared_mutex* useLock(uintptr_t ref, size_t threadIndex);
	void freeLock(uintptr_t ref, size_t threadIndex);


	bool assertNoLocks(size_t threadIndex);
	bool assertNoLocks();

private:

	static const uintptr_t NIL = static_cast<uintptr_t>(NULL);

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
	uintptr_t FLAG_MASK;
	uintptr_t EXTRACT_COUNT_MASK;

	std::vector<std::atomic<size_t>> lockInUse_;
	std::vector<std::atomic<size_t>> lockIndex_;
	std::vector<std::atomic<uintptr_t>> refs_;
	std::vector<boost::shared_mutex> locks_;

	inline void countDown(size_t refIndex);
	inline bool countUpIfMatch(size_t refIndex, uintptr_t ref);
};

using namespace std;

ExternalFixedSizeLocksTable::ExternalFixedSizeLocksTable() : nThreads_(0), FLAG_MASK(0),
		EXTRACT_COUNT_MASK(0), lockInUse_(0), lockIndex_(0), refs_(0), locks_(0) { }

ExternalFixedSizeLocksTable::ExternalFixedSizeLocksTable(size_t nThreads) : nThreads_(nThreads),
	lockInUse_(REFS_PER_THREAD * nThreads), lockIndex_(REFS_PER_THREAD * nThreads),
	refs_(REFS_PER_THREAD * nThreads), locks_(REFS_PER_THREAD * nThreads) {
	assert (nThreads > 0);

	for (unsigned i = 0; i < REFS_PER_THREAD * nThreads; ++i) {
		lockInUse_[i] = false;
		refs_[i] = NIL;
		lockIndex_[i] = STATUS_INVALID;
	}

	size_t nMsbBitsShift = sizeof (nThreads) * 8 - __builtin_clzl(nThreads);
	assert (nMsbBitsShift < (1uL << (sizeof (size_t) * 8 - 1uL)));
	EXTRACT_COUNT_MASK = (1uL << nMsbBitsShift) - 1uL;
	FLAG_MASK = ~EXTRACT_COUNT_MASK;
}

boost::shared_mutex* ExternalFixedSizeLocksTable::findLock(uintptr_t ref, size_t threadIndex) {
	assert (nThreads_ > 0);
	// finds a new lock by checking if the given reference (and thus also the lock) is
	// already held by another thread. If so it is reused and the usage counter is increased.
	// Otherwise a new lock is acquired.
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

void ExternalFixedSizeLocksTable::countDown(size_t refIndex) {
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

bool ExternalFixedSizeLocksTable::countUpIfMatch(size_t refIndex, uintptr_t wantRef) {
	if (wantRef == NIL) return false;

	assert ((wantRef & EXTRACT_COUNT_MASK) == 0);
	bool success;
	uintptr_t current = wantRef;
	do {
		uintptr_t want = current + 1;
#ifndef NDEBUG
			const unsigned int currentCount = want & EXTRACT_COUNT_MASK;
			assert (currentCount < nThreads_);
			assert ((want & FLAG_MASK) == wantRef);
#endif
		success = atomic_compare_exchange_strong(&refs_[refIndex], &current, want);

		if (!success && (current & FLAG_MASK) != wantRef) {
			// the reference at the given index did not match the desired reference
			return false;
		}
	} while (!success);

	assert ((refs_[refIndex] & EXTRACT_COUNT_MASK) > 0);
	assert ((wantRef != NIL));
	return true;
}

boost::shared_mutex* ExternalFixedSizeLocksTable::useLock(uintptr_t ref, size_t threadIndex) {
	// check which of the two stored references to use
	// do not change the lock, the index, the state or the counter

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

void ExternalFixedSizeLocksTable::freeLock(uintptr_t ref, size_t threadIndex) {
	// frees the reference of the given node
	// that is it (1) waits until the proper reference is not flagged any more,
	// (2) sets the status to invalid and (3) reduces the usage counter

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

bool ExternalFixedSizeLocksTable::assertNoLocks(size_t threadIndex) {
	size_t offset = REFS_PER_THREAD * threadIndex;

	for (unsigned j = 0; j < REFS_PER_THREAD; ++j) {
		assert (lockIndex_[offset + j] == STATUS_INVALID);
		assert (refs_[offset + j] == NIL);
	}

	return true;
}

bool ExternalFixedSizeLocksTable::assertNoLocks() {

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

#endif /* SRC_UTIL_EXTERNALFIXEDSIZELOCKTABLE_H_ */
