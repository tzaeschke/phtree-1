/*
 * OptimisticLockCoupling.h
 *
 *  Created on: Sep 20, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_OPTIMISTICLOCKCOUPLING_H_
#define SRC_UTIL_OPTIMISTICLOCKCOUPLING_H_

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class OptimisticLockCoupling {
public:
	static uint64_t readLock(Node<DIM>* node, bool* restart);
	static void readUnlock(Node<DIM>* node, uint64_t version, bool* restart);
	static void upgradeToWriteLock(Node<DIM>* node, uint64_t version, bool* restart);
	static void upgradeToWriteLock(Node<DIM>* child, uint64_t childVersion,
			Node<DIM>* parent, uint64_t parentVersion, bool* restart);
	static void writeLock(Node<DIM>* node, bool* restart);
	static void writeUnlock(Node<DIM>* node);
	static void writeUnlockUnchanged(Node<DIM>* node);
	static void writeUnlockRemove(Node<DIM>* node);
	static bool isRemoved(Node<DIM>* node);
	static void checkVersion(Node<DIM>* node, uint64_t version, bool* restart);

private:

	static const uint64_t removedMask = 1uL;
	static const uint64_t lockedMask = 2uL;

	static bool isLocked(uint64_t version);

	static uint64_t setLocked(uint64_t version);
	static uint64_t unsetLocked(uint64_t version);
	static bool isRemoved(uint64_t version);
	static uint64_t awaitUnlock(Node<DIM>* node);
};

using namespace std;

#include <assert.h>
#include "nodes/Node.h"
#include <atomic>
#include <thread>

template <unsigned int DIM>
uint64_t OptimisticLockCoupling<DIM>::readLock(Node<DIM>* node, bool* restart) {
	uint64_t version = awaitUnlock(node);
	(*restart) = isRemoved(version);
	return version;
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::readUnlock(Node<DIM>* node,
		uint64_t version, bool* restart) {
	(*restart) = atomic_load(&(node->version)) != version;
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::checkVersion(Node<DIM>* node, uint64_t version, bool* restart) {
	readUnlock(node, version, restart);
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::upgradeToWriteLock(Node<DIM>* node,
		uint64_t version, bool* restart) {
	assert (!isRemoved(version));
	assert (!isLocked(version));
	(*restart) = !atomic_compare_exchange_strong(&(node->version), &version, setLocked(version));
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::upgradeToWriteLock(Node<DIM>* child, uint64_t childVersion,
		Node<DIM>* parent, uint64_t parentVersion, bool* restart) {
	upgradeToWriteLock(parent, parentVersion, restart);
	if (!(*restart)) {
		upgradeToWriteLock(child, childVersion, restart);
		if (*restart) {
			writeUnlockUnchanged(parent);
		}
	}
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::writeLock(Node<DIM>* node, bool* restart) {
	uint64_t version;
	bool retry = false;
	do {
		version = readLock(node, restart);
		if (!(*restart)) {
			upgradeToWriteLock(node, &retry);
		}
	} while (retry);
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::writeUnlock(Node<DIM>* node) {
	// increase the counter and unset the locked bit
	uint64_t version = atomic_load(&(node->version));
	assert (isLocked(version));
	assert (!isRemoved(version));
	bool success = atomic_compare_exchange_strong(&(node->version), &version, version + 2);
	assert (success);
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::writeUnlockRemove(Node<DIM>* node) {
	// sets the removed bit and increases the counter by overflowing the locked bit
	uint64_t version = atomic_load(&(node->version));
	assert (isLocked(version));
	assert (!isRemoved(version));
	bool success = atomic_compare_exchange_strong(&(node->version), &version, version + 3);
	assert (success);
}

template<unsigned int DIM>
void OptimisticLockCoupling<DIM>::writeUnlockUnchanged(Node<DIM>* node) {
	// sets the removed bit and increases the counter by overflowing the locked bit
	uintptr_t version = atomic_load(&(node->version));
	assert (isLocked(version));
	assert (!isRemoved(version));
	bool success = atomic_compare_exchange_strong(&(node->version), &version, unsetLocked(version));
	assert (success);
}

template<unsigned int DIM>
bool  OptimisticLockCoupling<DIM>::isRemoved(uint64_t version) {
	return (version & removedMask) != 0;
}

template<unsigned int DIM>
bool  OptimisticLockCoupling<DIM>::isRemoved(Node<DIM>* node) {
	uint64_t version = atomic_load(&(node->version));
	return isRemoved(version);
}

template<unsigned int DIM>
bool  OptimisticLockCoupling<DIM>::isLocked(uint64_t version) {
	return (version & lockedMask) != 0;
}

template<unsigned int DIM>
uint64_t OptimisticLockCoupling<DIM>::setLocked(uint64_t version) {
	return version | lockedMask;
}

template<unsigned int DIM>
uint64_t OptimisticLockCoupling<DIM>::unsetLocked(uint64_t version) {
	return version & (~lockedMask);
}

template<unsigned int DIM>
uint64_t OptimisticLockCoupling<DIM>::awaitUnlock(Node<DIM>* node) {
	uint64_t version = atomic_load(&(node->version));
	while (isLocked(version)) { // spin
		this_thread::yield();
		version = atomic_load(&(node->version));
	}

	return version;
}

#endif /* SRC_UTIL_OPTIMISTICLOCKCOUPLING_H_ */
