/*
 * DynamicNodeOperationsUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_
#define SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_

#ifndef BOOST_THREAD_VERSION
	// requires upwards conversions of shared mutex
#define BOOST_THREAD_VERSION 3
#endif

#include <atomic>
#include "nodes/NodeAddressContent.h"
#include "util/DeletedNodes.h"
#include "util/EntryTreeMap.h"

template <unsigned int DIM, unsigned int WIDTH>
class Entry;
template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class PHTree;
template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer;
template <unsigned int DIM, unsigned int WIDTH>
class EntryBufferPool;

template <unsigned int DIM, unsigned int WIDTH>
class DynamicNodeOperationsUtil {
public:

	static unsigned int nInsertSplitSuffix;
	static unsigned int nInsertSuffix;
	static unsigned int nInsertSuffixEnlarge;
	static unsigned int nInsertSuffixBuffer;
	static unsigned int nInsertSuffixIntoBuffer;
	static unsigned int nInsertSplitPrefix;
	static unsigned int nFlushCountWithin;
	static unsigned int nFlushCountAfter;

	static unsigned int nThreads;

	static atomic<unsigned long> nRestartReadRecurse;
	static atomic<unsigned long> nRestartWriteSplitPrefix;
	static atomic<unsigned long> nRestartWriteFLushBuffer;
	static atomic<unsigned long> nRestartInsertBuffer;
	static atomic<unsigned long> nRestartWriteSwapSuffix;
	static atomic<unsigned long> nRestartWriteInsertSuffixEnlarge;
	static atomic<unsigned long> nRestartWriteInsertSuffix;

	static void resetCounters();
	static void insert(const Entry<DIM, WIDTH>& e, PHTree<DIM, WIDTH>& tree);
	static void parallelInsert(const Entry<DIM, WIDTH>& e, PHTree<DIM, WIDTH>& tree);
	static void bulkInsert(const std::vector<Entry<DIM, WIDTH>>& entries, PHTree<DIM, WIDTH>& tree);
	static bool parallelBulkInsert(const Entry<DIM, WIDTH>& e, PHTree<DIM, WIDTH>& tree,
			EntryBufferPool<DIM, WIDTH>& pool, DeletedNodes<DIM>& deletedNodes, EntryTreeMap<DIM,WIDTH>& entryTreeMap);

	static void createSubnodeWithExistingSuffix(size_t currentIndex, Node<DIM>* currentNode,
			const NodeAddressContent<DIM>& content, const Entry<DIM, WIDTH>& entry,
			PHTree<DIM, WIDTH>& tree);
	static void swapSuffixWithBuffer(size_t currentIndex, Node<DIM>* currentNode,
			const NodeAddressContent<DIM>& content, const Entry<DIM, WIDTH>& entry,
			EntryBuffer<DIM, WIDTH>* buffer, PHTree<DIM, WIDTH>& tree);
	static Node<DIM>* insertSuffix(size_t currentIndex, size_t hcAddress, Node<DIM>* currentNode,
			const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree);
	static void splitSubnodePrefix(size_t currentIndex, size_t newPrefixLength, size_t oldPrefixLength,
			Node<DIM>* currentNode, const NodeAddressContent<DIM>& content, const Entry<DIM, WIDTH>& entry,
			PHTree<DIM, WIDTH>& tree);

	static void flushSubtree(EntryBuffer<DIM, WIDTH>* buffer, bool deallocate);
private:

	static inline bool needToCopyNodeForSuffixInsertion(Node<DIM>* currentNode);
	static inline bool optimisticWriteLock(Node<DIM>* node);
	static inline bool optimisticWriteLock(Node<DIM>* currentNode, Node<DIM>* previousNode);
	static inline void optimisticWriteUnlock(Node<DIM>* node);
	static inline void optimisticWriteUnlock(Node<DIM>* currentNode, Node<DIM>* previousNode);
	static inline void downgradeWriterToReader(Node<DIM>* node);
	static inline void readLockBlocking(Node<DIM>* node);
	static inline bool readLock(Node<DIM>* node);
	static inline void readUnlock(Node<DIM>* node);
	static inline void readUnlock(Node<DIM>* child, Node<DIM>* parent);
	static inline void readUnlock(Node<DIM>* child, Node<DIM>* current, Node<DIM>* parent);
	static inline bool tryWriteLockWithoutRead(Node<DIM>* node);
};

template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSplitPrefix = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSplitSuffix = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSuffix = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSuffixEnlarge = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nFlushCountWithin = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nFlushCountAfter = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSuffixBuffer = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSuffixIntoBuffer = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nThreads = 0;

template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartReadRecurse;
template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartWriteSplitPrefix;
template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartWriteFLushBuffer;
template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartInsertBuffer;
template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartWriteSwapSuffix;
template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartWriteInsertSuffixEnlarge;
template <unsigned int DIM, unsigned int WIDTH>
atomic<unsigned long> DynamicNodeOperationsUtil<DIM, WIDTH>::nRestartWriteInsertSuffix;

#include <assert.h>
#include <stdexcept>
#include <cstdint>
#include <set>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "util/SpatialSelectionOperationsUtil.h"
#include "util/NodeTypeUtil.h"
#include "util/MultiDimBitset.h"
#include "nodes/Node.h"
#include "nodes/NodeAddressContent.h"
#include "PHTree.h"
#include "nodes/TSuffixStorage.h"
#include "util/EntryBuffer.h"
#include "util/EntryBufferPool.h"

// requires upwards conversions of shared mutex
#define BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::resetCounters() {
	nInsertSplitPrefix = 0;
	nInsertSplitSuffix = 0;
	nInsertSuffix = 0;
	nInsertSuffixEnlarge = 0;
	nFlushCountAfter = 0;
	nFlushCountWithin = 0;
	nInsertSuffixBuffer = 0;
	nInsertSuffixIntoBuffer = 0;

	nRestartReadRecurse = 0;
	nRestartWriteSplitPrefix = 0;
	nRestartWriteFLushBuffer = 0;
	nRestartInsertBuffer = 0;
	nRestartWriteSwapSuffix = 0;
	nRestartWriteInsertSuffixEnlarge = 0;
	nRestartWriteInsertSuffix = 0;
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::createSubnodeWithExistingSuffix(
		size_t currentIndex, Node<DIM>* currentNode, const NodeAddressContent<DIM>& content,
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {

#ifdef PRINT
	cout << "create subnode with existing suffix" << endl;
#endif

	++DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSplitSuffix;

	const size_t currentSuffixBits = DIM * (WIDTH - currentIndex - 1);
	const unsigned long* suffixStartBlock = content.getSuffixStartBlock();
	// create a temporary storage for the new prefix (all blocks are 0 filled)
	unsigned long prefixTmp[1 + (DIM * WIDTH - 1) / (sizeof(unsigned long) * 8)] = {};
	// 1. calculate the longest common prefix between the entry and the current suffix
	const size_t prefixLength = MultiDimBitset<DIM>::calculateLongestCommonPrefix(
			entry.values_, DIM * WIDTH, currentIndex + 1, suffixStartBlock, currentSuffixBits,
			prefixTmp);
	const size_t newSuffixLength = WIDTH - (currentIndex + 1 + prefixLength + 1);
	const size_t newSuffixBits = newSuffixLength * DIM;

	// 2. create a new node that stores the remaining suffix and the new entry: 2 suffixes
	Node<DIM>* subnode = NodeTypeUtil<DIM>::template buildNodeWithSuffixes<WIDTH>(prefixLength * DIM, 2, 2, newSuffixBits);
	assert (MultiDimBitset<DIM>::checkRangeUnset(
					subnode->getFixPrefixStartBlock(),
					subnode->getMaxPrefixLength(), 0));
	currentNode->insertAtAddress(content.address, subnode);

	// 3. copy the prefix into the subnode
	if (prefixLength > 0) {
		MultiDimBitset<DIM>::duplicateHighestBits(prefixTmp,
				prefixLength * DIM, prefixLength, subnode->getPrefixStartBlock());
	}

	// 4. paste the new suffixes into the new subnode
	// addresses in the subnode starts after common prefix
	const long insertEntryHCAddress = MultiDimBitset<DIM>::interleaveBits(entry.values_, currentIndex + 1 + prefixLength, entry.nBits_);
	const long existingEntryHCAddress = MultiDimBitset<DIM>::interleaveBits(suffixStartBlock, prefixLength, currentSuffixBits);
	assert(insertEntryHCAddress != existingEntryHCAddress); // otherwise there would have been a longer prefix

	// 5. add remaining bits after prefix and addresses as suffixes
	// TODO what if suffix length == 0?!
	const bool storeSuffixInNode = subnode->canStoreSuffixInternally(newSuffixLength * DIM);

	// create the required suffix blocks for both entries and insert a reference into the subnode
	if (storeSuffixInNode) {
		// insert both suffixes internally in the nodes
		unsigned long insertEntrySuffix = 0uL;
		unsigned long existingEntrySuffix = 0uL;
		// trim the existing entry's suffix by the common prefix length
		MultiDimBitset<DIM>::removeHighestBits(suffixStartBlock, currentSuffixBits, prefixLength + 1, &existingEntrySuffix);
		// insert the last bits of the new entry
		MultiDimBitset<DIM>::removeHighestBits(entry.values_, DIM * WIDTH, currentIndex + 1 + prefixLength + 1, &insertEntrySuffix);
		subnode->insertAtAddress(insertEntryHCAddress, insertEntrySuffix, entry.id_);
		subnode->insertAtAddress(existingEntryHCAddress, existingEntrySuffix, content.id);
		// TODO no need to create external suffix storage if it is stored internally anyway
		NodeTypeUtil<DIM>::template shrinkSuffixStorageIfPossible<WIDTH>(subnode);
	} else {
		assert (subnode->canStoreSuffix(2 * newSuffixBits) == 0);

		// insert the new and the old suffix by reserving memory in the new node
		const pair<unsigned long*, unsigned int> insertEntrySuffixStartBlock = subnode->reserveSuffixSpace(newSuffixBits);
		// insert the last bits of the new entry
		MultiDimBitset<DIM>::removeHighestBits(entry.values_, DIM * WIDTH, currentIndex + 1 + prefixLength + 1, insertEntrySuffixStartBlock.first);
		subnode->insertAtAddress(insertEntryHCAddress, insertEntrySuffixStartBlock.second, entry.id_);

		// move the previous suffix to the new subnode and remove shared bits
		const pair<unsigned long*, unsigned int> existingEntrySuffixStartBlock = subnode->reserveSuffixSpace(newSuffixBits);
		MultiDimBitset<DIM>::removeHighestBits(suffixStartBlock, currentSuffixBits, prefixLength + 1, existingEntrySuffixStartBlock.first);
		subnode->insertAtAddress(existingEntryHCAddress, existingEntrySuffixStartBlock.second, content.id);
	}

	// remove the old suffix if necessary
	assert (!content.hasSubnode);
	if (!content.directlyStoredSuffix) {
		// remove the previous suffix from the current node
		unsigned long* oldSuffixLocation = const_cast<unsigned long*>(suffixStartBlock);
		currentNode->freeSuffixSpace(currentSuffixBits, oldSuffixLocation);
		NodeTypeUtil<DIM>::template shrinkSuffixStorageIfPossible<WIDTH>(currentNode);
	}

	// no need to adjust the size of the node because the correct node type was already provided
	assert (currentNode->lookup(content.address, true).subnode == subnode);
	assert (MultiDimBitset<DIM>::checkRangeUnset(
			subnode->getFixPrefixStartBlock(),
			subnode->getMaxPrefixLength(),
			prefixLength * DIM));
	assert (!currentNode->getSuffixStorage()
			|| currentNode->getNStoredSuffixes() == currentNode->getSuffixStorage()->getNStoredSuffixes(currentSuffixBits));
	assert (subnode->getNStoredSuffixes() == 2
			&& (!subnode->getSuffixStorage()
					|| subnode->getSuffixStorage()->getNStoredSuffixes(newSuffixBits) == 2));
//TODO not possible for parallel insert:	assert (tree.lookup(entry).first);
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::swapSuffixWithBuffer(size_t currentIndex, Node<DIM>* currentNode,
			const NodeAddressContent<DIM>& content, const Entry<DIM, WIDTH>& entry,
			EntryBuffer<DIM, WIDTH>* buffer, PHTree<DIM, WIDTH>& tree) {
	assert (buffer);
	assert (content.exists && !content.hasSubnode);
	assert (buffer->assertCleared());

#ifdef PRINT
	cout << "swap suffix with a buffer" << endl;
#endif

	++nInsertSuffixBuffer;

	// 1. move the entire current suffix into the buffer
	const size_t suffixLength = WIDTH - currentIndex - 1;
	assert (suffixLength > 0);
	Entry<DIM, WIDTH>* newSuffixStorageEntry = buffer->init(suffixLength, currentNode, content.address);
	newSuffixStorageEntry->id_ = content.id;
	MultiDimBitset<DIM>::duplicateLowestBitsAligned(content.getSuffixStartBlock(),
			suffixLength * DIM, newSuffixStorageEntry->values_);

	// 2. insert the new entry into the buffer
	assert (!buffer->full());
	buffer->insert(entry);
	assert (!buffer->full());

	// 3. insert the buffer into the node
	const uintptr_t bufferRef = reinterpret_cast<uintptr_t>(buffer);
	currentNode->insertAtAddress(content.address, bufferRef);

	// 4. the locally stored suffix will not be needed again as it was copied into the buffer
	// --> remove it from the suffix storage since it will be replaced by a subtree (node reference)
	if (!content.directlyStoredSuffix) {
		unsigned long* oldSuffixLocation = const_cast<unsigned long*>(content.suffixStartBlock);
		currentNode->freeSuffixSpace(suffixLength * DIM, oldSuffixLocation);
		NodeTypeUtil<DIM>::template shrinkSuffixStorageIfPossible<WIDTH>(currentNode);
	}
}

template <unsigned int DIM, unsigned int WIDTH>
bool DynamicNodeOperationsUtil<DIM, WIDTH>::needToCopyNodeForSuffixInsertion(Node<DIM>* currentNode) {
	return currentNode->getNumberOfContents() == currentNode->getMaximumNumberOfContents();
}

template <unsigned int DIM, unsigned int WIDTH>
Node<DIM>* DynamicNodeOperationsUtil<DIM, WIDTH>::insertSuffix(size_t currentIndex,
		size_t hcAddress, Node<DIM>* currentNode,
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {
#ifdef PRINT
	cout << "inserting suffix";
#endif
	++nInsertSuffix;
	// TODO reuse this method in other two cases!
	Node<DIM>* adjustedNode = currentNode;
	if (currentNode->getNumberOfContents() == currentNode->getMaximumNumberOfContents()) {
		// need to adjust the node to insert another entry
		++nInsertSuffixEnlarge;
		adjustedNode = NodeTypeUtil<DIM>::copyIntoLargerNode(currentNode->getMaximumNumberOfContents() + 1, currentNode);
#ifdef PRINT
	cout << " (enlarged node from " << currentNode->getMaximumNumberOfContents() << " to " << adjustedNode->getMaximumNumberOfContents() << ")";
#endif
	}
	assert(adjustedNode->getNumberOfContents() < adjustedNode->getMaximumNumberOfContents());

	const size_t suffixBits= DIM * (WIDTH - (currentIndex + 1));
	if (adjustedNode->canStoreSuffixInternally(suffixBits)) {
		unsigned long suffix = 0uL;
		MultiDimBitset<DIM>::removeHighestBits(entry.values_, DIM * WIDTH, currentIndex + 1, &suffix);
		adjustedNode->insertAtAddress(hcAddress, suffix, entry.id_);
#ifdef PRINT
	cout << " internally";
#endif
	} else {
		unsigned int newTotalSuffixBlocks = adjustedNode->canStoreSuffix(suffixBits);
		if (newTotalSuffixBlocks != 0) {
			NodeTypeUtil<DIM>::template enlargeSuffixStorage<WIDTH>(newTotalSuffixBlocks, adjustedNode);
			assert (adjustedNode->canStoreSuffix(suffixBits) == 0);
#ifdef PRINT
	cout << " (enlarged suffix storage to " << adjustedNode->getSuffixStorage()->getNMaxStorageBlocks() << " block(s) )";
#endif
		}

		const pair<unsigned long*, unsigned int> suffixStartBlock = adjustedNode->reserveSuffixSpace(suffixBits);
		adjustedNode->insertAtAddress(hcAddress, suffixStartBlock.second, entry.id_);
		MultiDimBitset<DIM>::removeHighestBits(entry.values_, DIM * WIDTH, currentIndex + 1, suffixStartBlock.first);
		assert(adjustedNode->lookup(hcAddress, true).suffixStartBlock == suffixStartBlock.first);
	}

	assert(adjustedNode);
	assert(adjustedNode->lookup(hcAddress, true).exists);
	assert(adjustedNode->lookup(hcAddress, true).id == entry.id_);

#ifdef PRINT
	cout << endl;
#endif

	return adjustedNode;
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::splitSubnodePrefix(
		size_t currentIndex, size_t newPrefixLength, size_t oldPrefixLength,
		Node<DIM>* currentNode,	const NodeAddressContent<DIM>& content,
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {
	assert (newPrefixLength < oldPrefixLength && oldPrefixLength > 0);

#ifdef PRINT
	cout << "split subnode prefix" << endl;
#endif

	++nInsertSplitPrefix;

	const Node<DIM>* oldSubnode = content.subnode;
	assert (oldSubnode->getPrefixLength() == oldPrefixLength);
	assert (MultiDimBitset<DIM>::checkRangeUnset(
			oldSubnode->getFixPrefixStartBlock(),
			oldSubnode->getMaxPrefixLength(),
			oldPrefixLength * DIM));
	const size_t newSubnodeSuffixLength = WIDTH - (currentIndex + 1 + newPrefixLength + 1);
	// build a node that will hold 1 subnode and 1 suffix
	Node<DIM>* newSubnode = NodeTypeUtil<DIM>::template buildNodeWithSuffixes<WIDTH>(DIM * newPrefixLength, 2, 1, DIM * newSubnodeSuffixLength);
	currentNode->insertAtAddress(content.address, newSubnode);

	const unsigned long newSubnodeEntryHCAddress =
			MultiDimBitset<DIM>::interleaveBits(entry.values_, currentIndex + 1 + newPrefixLength, DIM * WIDTH);
	const unsigned long newSubnodePrefixDiffHCAddress =
		MultiDimBitset<DIM>::interleaveBits(oldSubnode->getFixPrefixStartBlock(), newPrefixLength, oldPrefixLength * DIM);
	assert (newSubnodeEntryHCAddress != newSubnodePrefixDiffHCAddress);

	// insert remaining entry bits as suffix in the new subnode
	insertSuffix(currentIndex + 1 + newPrefixLength, newSubnodeEntryHCAddress, newSubnode, entry, tree);

	// move A part of the old prefix to the new subnode and remove [A | d] from the old prefix
	if (newPrefixLength > 0) {
		MultiDimBitset<DIM>::duplicateHighestBits(oldSubnode->getFixPrefixStartBlock(), DIM * oldPrefixLength,
				newPrefixLength, newSubnode->getPrefixStartBlock());
	}

	// move remaining d part to a copy of the old subnode
	const size_t remainingOldPrefixBits = DIM * (oldPrefixLength - newPrefixLength - 1);
	// TODO if the number of prefix blocks does not change: no need to duplicate block
	Node<DIM>* oldSubnodeCopy = NodeTypeUtil<DIM>::copyWithoutPrefix(remainingOldPrefixBits, oldSubnode);
	if (remainingOldPrefixBits > 0) {
		MultiDimBitset<DIM>::removeHighestBits(oldSubnode->getFixPrefixStartBlock(),
				oldPrefixLength * DIM, newPrefixLength + 1, oldSubnodeCopy->getPrefixStartBlock());
	}

	// replace the old subnode with the copy
	newSubnode->insertAtAddress(newSubnodePrefixDiffHCAddress, oldSubnodeCopy);

	assert (currentNode->lookup(content.address, true).hasSubnode);
	assert (currentNode->lookup(content.address, true).subnode == newSubnode);
	assert (newSubnode->lookup(newSubnodeEntryHCAddress, true).exists);
	assert (!newSubnode->lookup(newSubnodeEntryHCAddress, true).hasSubnode);
	assert (newSubnode->lookup(newSubnodeEntryHCAddress, true).id == entry.id_);
	assert (newSubnode->lookup(newSubnodePrefixDiffHCAddress, true).hasSubnode);
	assert (newSubnode->lookup(newSubnodePrefixDiffHCAddress, true).subnode == oldSubnodeCopy);
	assert (MultiDimBitset<DIM>::checkRangeUnset(
			oldSubnodeCopy->getFixPrefixStartBlock(),
			oldSubnodeCopy->getMaxPrefixLength(),
			remainingOldPrefixBits));
// TODO not possible for parallel insert:	assert (tree.lookup(entry).first);


// TODO do not delete because of parallel workers: delete oldSubnode;

	// no need to adjust size because the old node remains and the new
	// one already has the correct size
}

template <unsigned int DIM, unsigned int WIDTH>
bool DynamicNodeOperationsUtil<DIM, WIDTH>::optimisticWriteLock(Node<DIM>* node) {
	assert (node);
	assert (!node->removed);
	assert (node->rwLock.state.shared_count > 0);

	bool success = false;
	if (node->rwLock.try_unlock_shared_and_lock_upgrade()) {
		// the thread now holds the only lock with upgrade privileges on the node
		node->rwLock.unlock_upgrade_and_lock();
		success = true;
	}

	return success;
}

template<unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::downgradeWriterToReader(Node<DIM>* node) {
	node->rwLock.unlock_and_lock_shared();
}

template<unsigned int DIM, unsigned int WIDTH>
bool DynamicNodeOperationsUtil<DIM, WIDTH>::optimisticWriteLock(
		Node<DIM>* child, Node<DIM>* parent) {

	if (optimisticWriteLock(parent)) {
		if (optimisticWriteLock(child)) {
			return true;
		} else {
			// downgrade the parent write lock to a shared lock again
			downgradeWriterToReader(parent);
		}
	}

	return false;
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::optimisticWriteUnlock(Node<DIM>* child, Node<DIM>* parent) {
	optimisticWriteUnlock(child);
	optimisticWriteUnlock(parent);
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::optimisticWriteUnlock(Node<DIM>* node) {
	assert (node);
	node->rwLock.unlock();
}

template <unsigned int DIM, unsigned int WIDTH>
bool DynamicNodeOperationsUtil<DIM, WIDTH>::tryWriteLockWithoutRead(Node<DIM>* node) {
	return node->rwLock.try_lock();
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::readLockBlocking(Node<DIM>* node) {
	return node->rwLock.lock_shared();
}

template <unsigned int DIM, unsigned int WIDTH>
bool DynamicNodeOperationsUtil<DIM, WIDTH>::readLock(Node<DIM>* node) {
	assert (node);
	return node->rwLock.try_lock_shared();
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::readUnlock(Node<DIM>* node) {
	assert (node);
	node->rwLock.unlock_shared();
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::readUnlock(Node<DIM>* child, Node<DIM>* parent) {
	readUnlock(child);
	if (parent) readUnlock(parent);
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::readUnlock(Node<DIM>* child, Node<DIM>* current, Node<DIM>* parent) {
	readUnlock(child);
	readUnlock(current);
	if (parent) readUnlock(parent);
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::parallelInsert(const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {
	size_t lastHcAddress, index;
	index = 0;
	Node<DIM>* lastNode = NULL;
	Node<DIM>* currentNode = NULL;
	NodeAddressContent<DIM> content;
	bool restart = true;

	while (index < WIDTH) {
		if (restart) {
			if (currentNode) readUnlock(currentNode);
			if (lastNode) readUnlock(lastNode);

			index = 0;
			lastHcAddress = 0;
			lastNode = NULL;
			currentNode = tree.root_;
			readLockBlocking(currentNode);
			restart = false;
		}

		const size_t currentIndex = index + currentNode->getPrefixLength();
		const unsigned long hcAddress =
				MultiDimBitset<DIM>::interleaveBits(entry.values_, currentIndex, WIDTH * DIM);
		// TODO create content once and populate after each iteration instead of creating a new one
		currentNode->lookup(hcAddress, content, true);
		assert(!content.exists || content.address == hcAddress);
		assert(!content.exists || !content.hasSpecialPointer);

		if (content.exists && content.hasSubnode) {
			// there is a subnode so the last node is not relevant any more
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }

			// node entry and subnode exist:
			// validate prefix of subnode
			// case 1 (entry contains prefix): recurse on subnode
			// case 2 (otherwise): split prefix at difference into two subnodes

			// need to get read access to the subnode
			Node<DIM>* subnode = content.subnode;
			if (readLock(subnode)) {
				const size_t subnodePrefixLength = subnode->getPrefixLength();
				bool prefixIncluded = true;
				size_t differentBitAtPrefixIndex = -1;
				if (subnodePrefixLength > 0) {
					const pair<bool, size_t> comp =  MultiDimBitset<DIM>::compare(entry.values_, DIM * WIDTH,
							currentIndex + 1, currentIndex + 1 + subnodePrefixLength,
							subnode->getFixPrefixStartBlock(), DIM * subnodePrefixLength);
					prefixIncluded = comp.first;
					differentBitAtPrefixIndex = comp.second;
				}

				lastNode = currentNode;
				currentNode = subnode;

				if (prefixIncluded) {
					// recurse on subnode
					#ifdef PRINT
						cout << "recurse -> ";
					#endif
					lastHcAddress = hcAddress;
					index = currentIndex + 1;
				} else {
					// split prefix of subnode [A | d | B] where d is the index of the first different bit
					// create new node with prefix A and only leave prefix B in old subnode
					if (optimisticWriteLock(currentNode, lastNode)) {
						splitSubnodePrefix(currentIndex, differentBitAtPrefixIndex, subnodePrefixLength, lastNode, content, entry, tree);
						optimisticWriteUnlock(currentNode, lastNode);
						currentNode->removed = true;
						delete currentNode;
						break;
					} else {
						restart = true;
					}
				}
			} else {
				// did not get access to the subnode so restart
				restart = true;
			}
		} else if (content.exists && !content.hasSubnode) {
			// node entry and suffix exist:
			// convert suffix to new node with prefix (longest common) + insert
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }

			if (optimisticWriteLock(currentNode)) {
				createSubnodeWithExistingSuffix(currentIndex, currentNode, content, entry, tree);
				optimisticWriteUnlock(currentNode);
				break;
			} else {
				restart = true;
			}
		} else if (lastNode && needToCopyNodeForSuffixInsertion(currentNode)) {
			// insert the suffix into a node that will be changed so needs to be reinserted into the parent
			if (optimisticWriteLock(currentNode, lastNode)) {
				Node<DIM>* adjustedNode = insertSuffix(currentIndex, hcAddress, currentNode, entry, tree);
				assert (adjustedNode && (adjustedNode != currentNode));
				lastNode->insertAtAddress(lastHcAddress, adjustedNode);
				optimisticWriteUnlock(currentNode, lastNode);
				currentNode->removed = true;
				delete currentNode;
				break;
			} else {
				restart = true;
			}
		} else {
			// inserting the suffix into a node that will not be changed
			// therefore, the last node is not needed any more
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }

			if (optimisticWriteLock(currentNode)) {
				Node<DIM>* adjustedNode = insertSuffix(currentIndex, hcAddress, currentNode, entry, tree);
				assert (adjustedNode && (adjustedNode == currentNode));
				optimisticWriteUnlock(currentNode);
				break;
			} else {
				restart = true;
			}
		}
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>& entry,
		PHTree<DIM, WIDTH>& tree) {

	size_t lastHcAddress = 0;
	size_t index = 0;
	Node<DIM>* lastNode = NULL;
	Node<DIM>* currentNode = tree.root_;
	NodeAddressContent<DIM> content;

	while (index < WIDTH) {

		const size_t currentIndex = index + currentNode->getPrefixLength();
		const unsigned long hcAddress =
				MultiDimBitset<DIM>::interleaveBits(entry.values_, currentIndex, WIDTH * DIM);
		// TODO create content once and populate after each iteration instead of creating a new one
		currentNode->lookup(hcAddress, content, true);
		assert(!content.exists || content.address == hcAddress);
		assert(!content.exists || !content.hasSpecialPointer);

		if (content.exists && content.hasSubnode) {
			// node entry and subnode exist:
			// validate prefix of subnode
			// case 1 (entry contains prefix): recurse on subnode
			// case 2 (otherwise): split prefix at difference into two subnodes
			const size_t subnodePrefixLength = content.subnode->getPrefixLength();
			bool prefixIncluded = true;
			size_t differentBitAtPrefixIndex = -1;
			if (subnodePrefixLength > 0) {
				const pair<bool, size_t> comp =  MultiDimBitset<DIM>::compare(entry.values_, DIM * WIDTH,
						currentIndex + 1, currentIndex + 1 + subnodePrefixLength,
						content.subnode->getFixPrefixStartBlock(), DIM * subnodePrefixLength);
				prefixIncluded = comp.first;
				differentBitAtPrefixIndex = comp.second;
			}

			if (prefixIncluded) {
				// recurse on subnode
				#ifdef PRINT
					cout << "recurse -> ";
				#endif
				lastHcAddress = hcAddress;
				lastNode = currentNode;
				currentNode = content.subnode;
				index = currentIndex + 1;
			} else {
				// split prefix of subnode [A | d | B] where d is the index of the first different bit
				// create new node with prefix A and only leave prefix B in old subnode
				splitSubnodePrefix(currentIndex, differentBitAtPrefixIndex, subnodePrefixLength, currentNode, content, entry, tree);
				break;
			}
		} else if (content.exists && !content.hasSubnode) {
			// node entry and suffix exist:
			// convert suffix to new node with prefix (longest common) + insert
			createSubnodeWithExistingSuffix(currentIndex, currentNode, content, entry, tree);
			break;
		} else {
			// node entry does not exist:
			// insert entry + suffix
			Node<DIM>* adjustedNode = insertSuffix(currentIndex,
					hcAddress, currentNode, entry, tree);
			assert(adjustedNode);
			if (adjustedNode != currentNode && lastNode) {
				// the subnode changed: store the new one and delete the old
				lastNode->insertAtAddress(lastHcAddress, adjustedNode);
				delete currentNode;
				currentNode = adjustedNode;
			} else if (adjustedNode != currentNode) {
				// the root node changed
				currentNode = adjustedNode;
				delete tree.root_;
				tree.root_ = adjustedNode;
				assert (tree.lookup(entry).first);
			}

			break;
		}
	}

	#ifndef NDEBUG
		// validation only: lookup again after insertion
		const size_t hcAddress =
						MultiDimBitset<DIM>::interleaveBits(entry.values_, index + currentNode->getPrefixLength(), WIDTH * DIM);
		currentNode->lookup(hcAddress, content, true);
		assert(content.exists && content.address == hcAddress
						&& "after insertion the entry is always contained at the address");
		pair<bool, int> retr = tree.lookup(entry);
		assert (retr.first && retr.second == entry.id_
			&& "after insertion the entry is always contained in the tree");
		// does the node store the minimum number of suffix blocks
		//const size_t remainingSuffixBits = DIM * (WIDTH - (index + currentNode->getPrefixLength() + 1));
		//const size_t blocksPerSuffix = 1 + (remainingSuffixBits - 1) / (8 * sizeof (unsigned long));
		//size_t suffixesInNode =
	#endif
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::bulkInsert(
		const std::vector<Entry<DIM, WIDTH>>& entries,
		PHTree<DIM, WIDTH>& tree) {

	Node<DIM>* currentRoot = tree.root_;
	NodeAddressContent<DIM> content;
	EntryBufferPool<DIM, WIDTH>* pool = new EntryBufferPool<DIM,WIDTH>();

	for (const auto &entry : entries) {
		size_t lastHcAddress = 0;
		size_t index = 0;
		Node<DIM>* lastNode = NULL;
		Node<DIM>* currentNode = currentRoot;

		while (index < WIDTH) {
			const size_t currentIndex = index + currentNode->getPrefixLength();
			const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(
					entry.values_, currentIndex, WIDTH * DIM);
			// TODO create content once and populate after each iteration instead of creating a new one
			currentNode->lookup(hcAddress, content, true);
			assert(!content.exists || content.address == hcAddress);

			if (content.exists && content.hasSubnode) {
				// node entry and subnode exist:
				// validate prefix of subnode
				// case 1 (entry contains prefix): recurse on subnode
				// case 2 (otherwise): split prefix at difference into two subnodes
				const size_t subnodePrefixLength =
						content.subnode->getPrefixLength();
				bool prefixIncluded = true;
				size_t differentBitAtPrefixIndex = -1;
				if (subnodePrefixLength > 0) {
					const pair<bool, size_t> comp =
							MultiDimBitset<DIM>::compare(entry.values_,
									DIM * WIDTH, currentIndex + 1,
									currentIndex + 1 + subnodePrefixLength,
									content.subnode->getFixPrefixStartBlock(),
									DIM * subnodePrefixLength);
					prefixIncluded = comp.first;
					differentBitAtPrefixIndex = comp.second;
				}

				if (prefixIncluded) {
					// recurse on subnode
					lastHcAddress = hcAddress;
					lastNode = currentNode;
					currentNode = content.subnode;
					index = currentIndex + 1;
				} else {
					// split prefix of subnode [A | d | B] where d is the index of the first different bit
					// create new node with prefix A and only leave prefix B in old subnode
					splitSubnodePrefix(currentIndex, differentBitAtPrefixIndex,
							subnodePrefixLength, currentNode, content, entry,
							tree);

					break;
				}
			} else if (content.exists && content.hasSpecialPointer) {
				// a buffer was found that can be filled
				EntryBuffer<DIM, WIDTH>* buffer = reinterpret_cast<EntryBuffer<DIM, WIDTH>*>(content.specialPointer);
				assert (buffer && !buffer->full());
				bool needFlush = buffer->insert(entry);
#ifdef PRINT
				cout << "insert into buffer (flush: " << needFlush << ")" << endl;
#endif
				if (needFlush) {
					flushSubtree(buffer, true);
					++nFlushCountWithin;
				}

				break;
			} else if (content.exists && !content.hasSubnode) {
				// instead of splitting the suffix a buffer is added
				EntryBuffer<DIM, WIDTH>* buffer = pool->allocate();
				if (!buffer) {
					pool->fullDeallocate();
					buffer = pool->allocate();
					assert (buffer);
				}

				swapSuffixWithBuffer(currentIndex, currentNode, content, entry, buffer, tree);
				break;
			} else {
				// node entry does not exist:
				// insert entry + suffix
				Node<DIM>* adjustedNode = insertSuffix(currentIndex, hcAddress,
						currentNode, entry, tree);
				assert(adjustedNode);
				if (adjustedNode != currentNode && lastNode) {
					// the subnode changed: store the new one and delete the old
					lastNode->insertAtAddress(lastHcAddress, adjustedNode);
					delete currentNode;
					currentNode = adjustedNode;
				} else if (adjustedNode != currentNode) {
					// the root node changed
					currentRoot = adjustedNode;
					// update the root node
					delete tree.root_;
					tree.root_ = currentRoot;
				}

				break;
			}
		}
	}

	// remove all buffers
	pool->fullDeallocate();
	delete pool;
}

template<unsigned int DIM, unsigned int WIDTH>
bool DynamicNodeOperationsUtil<DIM, WIDTH>::parallelBulkInsert(
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree,
		EntryBufferPool<DIM, WIDTH>& pool, DeletedNodes<DIM>& deletedNodes,
		EntryTreeMap<DIM,WIDTH>& entryTreeMap) {

#ifdef PRINT
		cout << entry.id_ << ": " << flush;
#endif

	entryTreeMap.compareForStart(entry);
	size_t lastHcAddress, index;
	index = 0;
	Node<DIM>* lastNode = NULL;
	Node<DIM>* currentNode = NULL;
	Node<DIM>* highestNode;
	NodeAddressContent<DIM> content;
	bool restart = true;

	while (index < WIDTH) {
		while (restart) {
			if (currentNode) { readUnlock(currentNode); }
			if (lastNode) { readUnlock(lastNode); }
			index = 0;
			lastHcAddress = 0;
			lastNode = NULL;
			entryTreeMap.getNextUndeletedNode(&highestNode, &index);
			currentNode = (highestNode)? highestNode : tree.root_;
			readLockBlocking(currentNode);
			restart = currentNode->removed;
		}

		assert (!lastNode || !lastNode->removed);
		assert (!currentNode->removed);
		const size_t currentIndex = index + currentNode->getPrefixLength();
		const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(entry.values_, currentIndex, WIDTH * DIM);
		// TODO create content once and populate after each iteration instead of creating a new one
		currentNode->lookup(hcAddress, content, true);
		assert(!content.exists || content.address == hcAddress);

		if (content.exists && content.hasSubnode) {
			// there is a subnode so the last node is not relevant any more
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }

			// node entry and subnode exist:
			// validate prefix of subnode
			// case 1 (entry contains prefix): recurse on subnode
			// case 2 (otherwise): split prefix at difference into two subnodes

			// need to get read access to the subnode
			Node<DIM>* subnode = content.subnode;
			if (readLock(subnode) && !subnode->removed) {
				const size_t subnodePrefixLength = subnode->getPrefixLength();
				bool prefixIncluded = true;
				size_t differentBitAtPrefixIndex = -1;
				if (subnodePrefixLength > 0) {
					const pair<bool, size_t> comp = MultiDimBitset<DIM>::compare(entry.values_,
									DIM * WIDTH, currentIndex + 1, currentIndex + 1 + subnodePrefixLength,
									subnode->getFixPrefixStartBlock(), DIM * subnodePrefixLength);
					prefixIncluded = comp.first;
					differentBitAtPrefixIndex = comp.second;
				}

				lastNode = currentNode;
				currentNode = subnode;

				if (prefixIncluded) {
					// recurse on subnode
#ifdef PRINT
					cout << "recurse (" << index << "-" << currentIndex << ") -> " << flush;
#endif
					lastHcAddress = hcAddress;
					index = currentIndex + 1;
					entryTreeMap.put(index, currentNode);
				} else {
					// split prefix of subnode [A | d | B] where d is the index of the first different bit
					// create new node with prefix A and only leave prefix B in old subnode
					if (optimisticWriteLock(currentNode, lastNode)) {
						splitSubnodePrefix(currentIndex, differentBitAtPrefixIndex, subnodePrefixLength, lastNode, content, entry, tree);
						deletedNodes.add(currentNode);
						optimisticWriteUnlock(currentNode, lastNode);
						break;
					} else {
						restart = true;
//						++nRestartWriteSplitPrefix;
					}
				}
			} else {
				// did not get access to the subnode so restart
				restart = true;
//				++nRestartReadRecurse;
			}
		} else if (content.exists && content.hasSpecialPointer) {
			// a buffer was found that can be filled
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }
			EntryBuffer<DIM, WIDTH>* buffer = reinterpret_cast<EntryBuffer<DIM,WIDTH>*>(content.specialPointer);
			if (buffer->full()) {
				if (optimisticWriteLock(currentNode)) {
					assert (buffer->full());
					// cleaning the old buffer and restart
					flushSubtree(buffer, true);
					downgradeWriterToReader(currentNode);
					// continue with the current node
				} else {
					restart = true;
//					++nRestartWriteFLushBuffer;
				}
			} else if (buffer->insert(entry)) {
				// successfully inserted the entry into the buffer
#ifdef PRINT
					cout << "inserted into buffer" << endl;
#endif
				readUnlock(currentNode);
				break;
			} else {
				// failed to insert into the buffer so restart
				restart = true;
//				++nRestartInsertBuffer;
			}
		} else if (content.exists && !content.hasSubnode) {
			// instead of splitting the suffix a buffer is added
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }
			EntryBuffer<DIM, WIDTH>* buffer = pool.allocate();

			if (!buffer) {
				// unable to allocate another buffer!
				// need to flush the pools first!
				readUnlock(currentNode);
				return false;
			}

			if (optimisticWriteLock(currentNode)) {
				swapSuffixWithBuffer(currentIndex, currentNode, content, entry, buffer, tree);
				optimisticWriteUnlock(currentNode);
				break;
			} else {
				pool.deallocate(buffer);
				restart = true;
//				++nRestartWriteSwapSuffix;
			}
		} else if (needToCopyNodeForSuffixInsertion(currentNode)) {
			if (!lastNode) {
				// the entry node changed so need to back up to a previous node
				entryTreeMap.enforcePreviousNode();
				restart = true;
			} else if (optimisticWriteLock(currentNode, lastNode)) {
				// insert the suffix into a node that will be changed so needs to be reinserted into the parent
				Node<DIM>* adjustedNode = insertSuffix(currentIndex, hcAddress, currentNode, entry, tree);
				assert(adjustedNode && (adjustedNode != currentNode));
				lastNode->insertAtAddress(lastHcAddress, adjustedNode);
				deletedNodes.add(currentNode);
				optimisticWriteUnlock(currentNode, lastNode);
				break;
			} else {
				restart = true;
//				++nRestartWriteInsertSuffixEnlarge;
			}
		} else {
			// inserting the suffix into a node that will not be changed
			// therefore, the last node is not needed any more
			if (lastNode) { readUnlock(lastNode); lastNode = NULL; }

			if (optimisticWriteLock(currentNode)) {
				Node<DIM>* adjustedNode = insertSuffix(currentIndex, hcAddress, currentNode, entry, tree);
				assert(adjustedNode && (adjustedNode == currentNode));
				optimisticWriteUnlock(currentNode);
				break;
			} else {
				restart = true;
//				++nRestartWriteInsertSuffix;
			}
		}
	}

	return true;
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::flushSubtree(
		EntryBuffer<DIM, WIDTH>* buffer, bool deallocate) {
	EntryBufferPool<DIM,WIDTH>* pool = buffer->getPool();
	assert (pool);
	buffer->flushToSubtree();
	buffer->clear();
	assert (buffer->assertCleared());
	if (deallocate) {
		pool->deallocate(buffer);
	}

}

#endif /* SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_ */
