/*
 * DynamicNodeOperationsUtil.h
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_
#define SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_

#include "nodes/NodeAddressContent.h"

template <unsigned int DIM, unsigned int WIDTH>
class Entry;
template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class PHTree;
template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer;

template <unsigned int DIM, unsigned int WIDTH>
class DynamicNodeOperationsUtil {
public:

	static unsigned int nInsertSplitSuffix;
	static unsigned int nInsertSuffix;
	static unsigned int nInsertSuffixEnlarge;
	static unsigned int nInsertSplitPrefix;

	static void insert(const Entry<DIM, WIDTH>& e, Node<DIM>* rootNode, PHTree<DIM, WIDTH>& tree);
	static void bulkInsert(const std::vector<const Entry<DIM, WIDTH>&>& entries, Node<DIM>* rootNode, PHTree<DIM, WIDTH>& tree);

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
};

template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSplitPrefix = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSplitSuffix = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSuffix = 0;
template <unsigned int DIM, unsigned int WIDTH>
unsigned int DynamicNodeOperationsUtil<DIM, WIDTH>::nInsertSuffixEnlarge = 0;

#include <assert.h>
#include <stdexcept>
#include <cstdint>
#include "util/SpatialSelectionOperationsUtil.h"
#include "util/NodeTypeUtil.h"
#include "util/MultiDimBitset.h"
#include "nodes/Node.h"
#include "nodes/NodeAddressContent.h"
#include "PHTree.h"
#include "nodes/TSuffixStorage.h"
#include "util/EntryBuffer.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::createSubnodeWithExistingSuffix(
		size_t currentIndex, Node<DIM>* currentNode, const NodeAddressContent<DIM>& content,
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {

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
	assert (tree.lookup(entry).first);
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::swapSuffixWithBuffer(size_t currentIndex, Node<DIM>* currentNode,
			const NodeAddressContent<DIM>& content, const Entry<DIM, WIDTH>& entry,
			EntryBuffer<DIM, WIDTH>* buffer, PHTree<DIM, WIDTH>& tree) {
	assert (buffer);
	assert (content.exists && !content.hasSubnode && !content.directlyStoredSuffix);

	// 1. move the entire current suffix into the buffer
	const size_t suffixLength = WIDTH - currentIndex - 1;
	assert (suffixLength > 0);
	unsigned long* newSuffixStorage = buffer->init(suffixLength);
	MultiDimBitset<DIM>::duplicateHighestBits(content.suffixStartBlock,
			suffixLength * DIM, suffixLength * DIM, newSuffixStorage);

	// 2. insert the new entry into the buffer
	assert (!buffer->full());
	buffer->insert(entry, currentIndex + 1);
	assert (!buffer->full());

	// 3. insert the buffer into the node
	const uintptr_t bufferRef = reinterpret_cast<uintptr_t>(buffer);
	currentNode->insertAtAddress(content.address, bufferRef);
}

template <unsigned int DIM, unsigned int WIDTH>
Node<DIM>* DynamicNodeOperationsUtil<DIM, WIDTH>::insertSuffix(size_t currentIndex,
		size_t hcAddress, Node<DIM>* currentNode,
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {
	++nInsertSuffix;
	// TODO reuse this method in other two cases!
	Node<DIM>* adjustedNode = currentNode;
	if (currentNode->getNumberOfContents() == currentNode->getMaximumNumberOfContents()) {
		// need to adjust the node to insert another entry
		++nInsertSuffixEnlarge;
		adjustedNode = NodeTypeUtil<DIM>::copyIntoLargerNode(currentNode->getMaximumNumberOfContents() + 1, currentNode);
	}
	assert(adjustedNode->getNumberOfContents() < adjustedNode->getMaximumNumberOfContents());

	const size_t suffixBits= DIM * (WIDTH - (currentIndex + 1));
	if (adjustedNode->canStoreSuffixInternally(suffixBits)) {
		unsigned long suffix = 0uL;
		MultiDimBitset<DIM>::removeHighestBits(entry.values_, DIM * WIDTH, currentIndex + 1, &suffix);
		adjustedNode->insertAtAddress(hcAddress, suffix, entry.id_);
	} else {
		unsigned int newTotalSuffixBlocks = adjustedNode->canStoreSuffix(suffixBits);
		if (newTotalSuffixBlocks != 0) {
			NodeTypeUtil<DIM>::template enlargeSuffixStorage<WIDTH>(newTotalSuffixBlocks, adjustedNode);
			assert (adjustedNode->canStoreSuffix(suffixBits) == 0);
		}

		const pair<unsigned long*, unsigned int> suffixStartBlock = adjustedNode->reserveSuffixSpace(suffixBits);
		adjustedNode->insertAtAddress(hcAddress, suffixStartBlock.second, entry.id_);
		MultiDimBitset<DIM>::removeHighestBits(entry.values_, DIM * WIDTH, currentIndex + 1, suffixStartBlock.first);
		assert(adjustedNode->lookup(hcAddress, true).suffixStartBlock == suffixStartBlock.first);
	}

	assert(adjustedNode);
	assert(adjustedNode->lookup(hcAddress, true).exists);
	assert(adjustedNode->lookup(hcAddress, true).id == entry.id_);

	return adjustedNode;
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::splitSubnodePrefix(
		size_t currentIndex, size_t newPrefixLength, size_t oldPrefixLength,
		Node<DIM>* currentNode,	const NodeAddressContent<DIM>& content,
		const Entry<DIM, WIDTH>& entry, PHTree<DIM, WIDTH>& tree) {
	assert (newPrefixLength < oldPrefixLength && oldPrefixLength > 0);

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
	assert (tree.lookup(entry).first);

	delete oldSubnode;
	// no need to adjust size because the old node remains and the new
	// one already has the correct size
}

template <unsigned int DIM, unsigned int WIDTH>
void DynamicNodeOperationsUtil<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>& entry,
		Node<DIM>* rootNode, PHTree<DIM, WIDTH>& tree) {

	size_t lastHcAddress = 0;
	size_t index = 0;
	Node<DIM>* lastNode = NULL;
	Node<DIM>* currentNode = rootNode;
	NodeAddressContent<DIM> content;

	while (index < WIDTH) {

		const size_t currentIndex = index + currentNode->getPrefixLength();
		const unsigned long hcAddress =
				MultiDimBitset<DIM>::interleaveBits(entry.values_, currentIndex, WIDTH * DIM);
		// TODO create content once and populate after each iteration instead of creating a new one
		currentNode->lookup(hcAddress, content, true);
		assert(!content.exists || content.address == hcAddress);
		assert(!content.hasSpecialPointer);

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
				#ifdef PRINT
					cout << "split subnode prefix" << endl;
				#endif
				// split prefix of subnode [A | d | B] where d is the index of the first different bit
				// create new node with prefix A and only leave prefix B in old subnode
				splitSubnodePrefix(currentIndex, differentBitAtPrefixIndex, subnodePrefixLength,
						currentNode, content, entry, tree);

				break;
			}

		} else if (content.exists && !content.hasSubnode) {
			#ifdef PRINT
				cout << "create subnode with existing suffix" << endl;
			#endif
			// node entry and suffix exist:
			// convert suffix to new node with prefix (longest common) + insert
			createSubnodeWithExistingSuffix(currentIndex,
					currentNode, content, entry, tree);

			break;
		} else {
			#ifdef PRINT
				cout << "insert suffix" << endl;
			#endif
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
		const std::vector<const Entry<DIM, WIDTH>&>& entries,
		Node<DIM>* rootNode,
		PHTree<DIM, WIDTH>& tree) {

	Node<DIM>* currentRoot = rootNode;
	NodeAddressContent<DIM> content;
	set<EntryBuffer<DIM, WIDTH>*> openBuffers;

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
			assert(!content.hasSpecialPointer);

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
				assert (openBuffers.count(buffer));
				assert (buffer && !buffer->full());
				bool needFlush = buffer->insert(entry, currentIndex);
				if (needFlush) {
					buffer->flushToSubtree(tree);
					openBuffers.erase(buffer);
					delete buffer;
				}

				break;
			} else if (content.exists && !content.hasSubnode) {
				// instead of splitting the suffix a buffer is added
				EntryBuffer<DIM, WIDTH>* buffer = new EntryBuffer<DIM, WIDTH>();
				assert (!openBuffers.count(buffer));
				swapSuffixWithBuffer(currentIndex, currentNode, content, entry, buffer, tree);
				openBuffers.insert(buffer);
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
	for (EntryBuffer<DIM, WIDTH>* buffer : openBuffers) {
		buffer->flushToSubtree(tree);
		delete buffer;
	}
}

#endif /* SRC_UTIL_DYNAMICNODEOPERATIONSUTIL_H_ */
