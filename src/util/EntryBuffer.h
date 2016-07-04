/*
 * EntryBuffer.h
 *
 *  Created on: Jun 30, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_ENTRYBUFFER_H_
#define SRC_UTIL_ENTRYBUFFER_H_

#include <assert.h>
#include <set>
#include "Entry.h"
#include "util/TEntryBuffer.h"

template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class PHTree;

template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer : public TEntryBuffer<DIM> {
public:
	EntryBuffer();
	~EntryBuffer() {};

	size_t getLongestCommonPrefix() const;
	bool insert(const Entry<DIM, WIDTH>& entry, unsigned int msbIndex);
	bool full() const;
	bool empty() const;
	bool hasPrefix() const;
	size_t size() const;
	size_t capacity() const;
	void clear();
	unsigned long* init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress, int id);
	void updateNode(Node<DIM>* node);
	std::pair<Node<DIM>*, unsigned long> getNodeAndAddress();
	void flushToSubtree(PHTree<DIM, WIDTH>& tree, set<EntryBuffer<DIM, WIDTH>>* globalBuffers);

private:
	static const size_t capacity_ = 50;

	size_t nextIndex_;
	size_t logestCommonPrefixLength_;
	size_t suffixBits_;
	int suffixId_;
	unsigned long suffixToCompareTo_[1 + (DIM * WIDTH - 1) / (8 * sizeof (unsigned long))];
	unsigned int lcpToSuffix_[capacity_];
	const Entry<DIM, WIDTH> buffer_[capacity_];

	inline void sortByPrefixLengthAscending();
};

#include "util/MultiDimBitset.h"
#include "util/NodeTypeUtil.h"
#include "nodes/Node.h"
#include "PHTree.h"

template <unsigned int DIM, unsigned int WIDTH>
EntryBuffer<DIM, WIDTH>::EntryBuffer() : nextIndex_(0), logestCommonPrefixLength_(0), suffixBits_(0),
	suffixId_(0), suffixToCompareTo_(), lcpToSuffix_(), buffer_() {
}

template <unsigned int DIM, unsigned int WIDTH>
unsigned long* EntryBuffer<DIM, WIDTH>::init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress, int id) {
	clear();
	assert (suffixLength <= (WIDTH - 1));
	suffixBits_ = suffixLength * DIM;
	suffixId_ = id;
	this->node_ = node;
	this->nodeHcAddress = hcAddress;
	return suffixToCompareTo_;
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::clear() {
	nextIndex_ = 0;
	logestCommonPrefixLength_ = WIDTH;
	suffixBits_ = 0;
	const size_t suffixBlocks = sizeof (suffixToCompareTo_) / sizeof (unsigned long);
	for (unsigned i = 0; i < suffixBlocks; ++i) {
		suffixToCompareTo_ = 0;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::size() const {
	return nextIndex_;
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::capacity() const {
	return capacity_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::full() const {
	assert (nextIndex_ <= capacity_);
	return nextIndex_ == capacity_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::empty() const {
	return nextIndex_ == 0;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::hasPrefix() const {
	assert (!empty());
	return logestCommonPrefixLength_ != 0;
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::getLongestCommonPrefix() const {
	assert (logestCommonPrefixLength_ < WIDTH);
	return logestCommonPrefixLength_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>& entry, unsigned int msbIndex) {
	assert (!full() && hasPrefix());

	buffer_[nextIndex_] = entry;
	const unsigned int nBitsToCompare = DIM * (WIDTH - msbIndex);
	const pair<bool, size_t> comp = MultiDimBitset<DIM>::compare(
			entry.values_, WIDTH * DIM, msbIndex, WIDTH, suffixToCompareTo_, suffixBits_);
	assert(!comp.first);
	lcpToSuffix_[nextIndex_] = comp.second;

	if (comp.second < logestCommonPrefixLength_) {
		logestCommonPrefixLength_ = comp.second;
	}

	++nextIndex_;
	// empty the buffer if it is full or if there is no more shared prefix
	return full() || !hasPrefix();
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::sortByPrefixLengthAscending() {
	// insertion sort efficient for small sets
	unsigned int i, j;
	for (i = 1; i < nextIndex_; i++) {
		const size_t tmp = lcpToSuffix_[i];
		const Entry<DIM, WIDTH> entry = buffer_[i];
		for (j = i; j >= 1 && tmp < lcpToSuffix_[j - 1]; j--)
			lcpToSuffix_[j] = lcpToSuffix_[j - 1];
		lcpToSuffix_[j] = tmp;
		buffer_[j] = entry;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::flushToSubtree(PHTree<DIM, WIDTH>& tree, set<EntryBuffer<DIM, WIDTH>>* globalBuffers) {
	assert (nextIndex_ > 0);

	sortByPrefixLengthAscending();

	Node<DIM>* currentNode = this->node_;
	unsigned long currentHcAddress = this->nodeHcAddress;
	unsigned int nEntries = 0;
	unsigned int lastLcp = 0;
	const unsigned int highestLcp = lcpToSuffix_[nextIndex_ - 1];
	const unsigned int index = WIDTH - suffixBits_ / DIM;
	vector<unsigned long> currentLevelAddresses;
	vector<bool> currentLevelAddressesUnique;
	vector<bool> currentLevelAddressesSameFirst;
	vector<EntryBuffer<DIM, WIDTH>*> newBuffers;

	for (unsigned i = 0; i < nextIndex_; i += nEntries) {
		const unsigned int currentLcp = lcpToSuffix_[i];
		assert (currentLcp > lastLcp);
		const unsigned int currentIndex = index + currentLcp;

		// count the number of entries that are directly inserted into the current node
		nEntries = 1;
		for (unsigned j = i + 1; j < nextIndex_ && lcpToSuffix_[j] == currentLcp; ++j) {
			++nEntries;
		}

		// extract all addresses
		for (unsigned j = i; j < i + nEntries; ++j) {
			const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(buffer_[j], currentIndex, DIM * WIDTH);
			currentLevelAddresses.push_back(hcAddress);
			currentLevelAddressesUnique.push_back(true);
			currentLevelAddressesSameFirst.push_back(false);
			newBuffers.push_back(NULL);
		}

		// count the number of unique addresses = #direct suffixes
		unsigned int nSuffixes = (currentLcp == highestLcp)? 1 : 0;
		for (unsigned j = 1; j < nEntries; ++j) {
			bool unique = true;
			unsigned k;
			for (k = 0; k < j && unique; k++) {
				unique = currentLevelAddresses[j] != currentLevelAddresses[k];
			}

			if (unique) {
				++nSuffixes;
			} else {
				if (!currentLevelAddressesSameFirst[k]) {
					newBuffers[k] = new EntryBuffer<DIM, WIDTH>();
					assert (nSuffixes > 0);
					--nSuffixes;
				}

				newBuffers[j] = newBuffers[k];
				currentLevelAddressesUnique[k] = false;
				currentLevelAddressesUnique[j] = false;
				currentLevelAddressesSameFirst[k] = true;
			}
		}
		assert (0 <= nSuffixes && nSuffixes <= nEntries);

		// create the new subnode of the correct size
		assert (nEntries > 0 && nSuffixes > 0 && nEntries >= nSuffixes);
		const unsigned int prefixLength = currentLcp - lastLcp;
		const unsigned int prefixBits = DIM * prefixLength;
		const unsigned int suffixBits = DIM * (WIDTH - (currentIndex + 1));
		Node<DIM>* newSubnode = NodeTypeUtil<DIM>::template buildNodeWithSuffixes<WIDTH>(
				prefixBits, nEntries, nSuffixes, suffixBits);
		currentNode->insertAtAddress(currentHcAddress, newSubnode);

		// copy prefix into the new node
		if (prefixLength > 0) {
			const size_t suffixBitsForCopy = suffixBits + DIM;
			MultiDimBitset<DIM>::duplicateHighestBits(suffixToCompareTo_, suffixBitsForCopy,
					prefixLength, newSubnode->getPrefixStartBlock());
		}

		// insert all suffixes into the new subnode
		for (unsigned j = 0; j < nEntries; ++j) {
			const unsigned long hcAddress = currentLevelAddresses[j];
			if (currentLevelAddressesUnique[j]) {
				// insert suffix directly into node
				// TODO actually no need to varify if the node is big enough!
				assert (!(newSubnode->lookup(hcAddress, true)).exists);
				Node<DIM>* adjustedNode = DynamicNodeOperationsUtil<DIM, WIDTH>::
						insertSuffix(currentIndex, hcAddress, newSubnode, buffer_[i + j], tree);
				assert (adjustedNode == newSubnode);
			} else if (currentLevelAddressesSameFirst[j]) {
				// inject suffix of entry into buffer and insert the buffer directly into the node
				assert (!(newSubnode->lookup(hcAddress, true)).exists);
				EntryBuffer<DIM, WIDTH>* newBuffer = newBuffers[j];
				unsigned long* newSuffixStorage = newBuffer->init((suffixBits / DIM), newSubnode, hcAddress, buffer_[i + j].id_);
				MultiDimBitset<DIM>::duplicateHighestBits(buffer_[i + j], suffixBits, suffixBits, newSuffixStorage);
				uintptr_t bufferRef = reinterpret_cast<uintptr_t>(newBuffer);
				newSubnode->insertAtAddress(hcAddress, bufferRef);
			} else {
				// insert entry into existing buffer
				assert ((newSubnode->lookup(hcAddress, true)).hasSpecialPointer);
				EntryBuffer<DIM, WIDTH>* existingBuffer = newBuffers[j];
				bool needFlush = existingBuffer->insert(buffer_[i + j], currentIndex);
				assert (!needFlush);
			}
		}

		currentNode = newSubnode;
		currentHcAddress = MultiDimBitset<DIM>::interleaveBits(suffixToCompareTo_, currentIndex + 1, DIM * WIDTH);
		lastLcp = currentLcp;
		currentLevelAddresses.clear();
		currentLevelAddressesUnique.clear();
		currentLevelAddressesSameFirst.clear();
		globalBuffers->insert(newBuffers.begin(), newBuffers.end());
		newBuffers.clear();
	}

	// insert the suffix into the lowest node
	assert (logestCommonPrefixLength_ * DIM < suffixBits_);
	const unsigned int currentIndex = index + highestLcp;
	currentHcAddress = MultiDimBitset<DIM>::interleaveBits(suffixToCompareTo_, currentIndex + 1, DIM * WIDTH);
	assert (!(currentNode->lookup(currentHcAddress, true)).exists);
	const unsigned int newSuffixBits = DIM * (WIDTH - (currentIndex + 1));
	if (currentNode->canStoreSuffixInternally(newSuffixBits)) {
		unsigned long suffix = 0uL;
		MultiDimBitset<DIM>::removeHighestBits(suffixToCompareTo_, suffixBits_, highestLcp, &suffix);
		currentNode->insertAtAddress(currentHcAddress, suffix, suffixId_);
	} else {
		assert (currentNode->canStoreSuffix(newSuffixBits) == 0);
		const pair<unsigned long*, unsigned int> suffixStartBlock = currentNode->reserveSuffixSpace(newSuffixBits);
		currentNode->insertAtAddress(currentHcAddress, suffixStartBlock.second, suffixId_);
		MultiDimBitset<DIM>::removeHighestBits(suffixToCompareTo_, suffixBits_, highestLcp, suffixStartBlock.first);
	}
}


#endif /* SRC_UTIL_ENTRYBUFFER_H_ */
