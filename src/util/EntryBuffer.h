/*
 * EntryBuffer.h
 *
 *  Created on: Jun 30, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_ENTRYBUFFER_H_
#define SRC_UTIL_ENTRYBUFFER_H_

#include <assert.h>
#include "Entry.h"

template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class PHTree;

template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer {
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
	unsigned long* init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress);
	void updateNode(Node<DIM>* node);
	std::pair<Node<DIM>*, unsigned long> getNodeAndAddress();
	void flushToSubtree(PHTree<DIM, WIDTH>& tree);

private:
	static const size_t capacity_ = 50;

	size_t nextIndex_;
	size_t logestCommonPrefixLength_;
	size_t suffixBits_;
	Node<DIM>* node_;
	unsigned long nodeHcAddress;
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
	node_(NULL), nodeHcAddress(0),
	suffixToCompareTo_(), lcpToSuffix_(), buffer_() {
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::getLongestCommonPrefix() const {
	return logestCommonPrefixLength_;
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::updateNode(Node<DIM>* node) {
	node_ = node;
}

template <unsigned int DIM, unsigned int WIDTH>
pair<Node<DIM>*, unsigned long> EntryBuffer<DIM, WIDTH>::getNodeAndAddress() {
	return pair<Node<DIM>*, unsigned long>(node_, nodeHcAddress);
}

template <unsigned int DIM, unsigned int WIDTH>
unsigned long* EntryBuffer<DIM, WIDTH>::init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress) {
	clear();
	assert (suffixLength <= (WIDTH - 1));
	suffixBits_ = suffixLength * DIM;
	node_ = node;
	nodeHcAddress = hcAddress;
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
void EntryBuffer<DIM, WIDTH>::flushToSubtree(PHTree<DIM, WIDTH>& tree) {

	sortByPrefixLengthAscending();

	Node<DIM>* currentNode = node_;
	unsigned long currentHcAddress = nodeHcAddress;
	unsigned int lastLcp = 0;
	unsigned int nSuffixes;
	const unsigned int index = WIDTH - suffixBits_ / DIM;

	for (unsigned i = 0; i < nextIndex_; i += nSuffixes) {
		const unsigned int currentLcp = lcpToSuffix_[i];

		// count the suffixes for the new subnode
		nSuffixes = 1;
		for (unsigned j = i + 1; j < nextIndex_ && lcpToSuffix_[j] == currentLcp; ++j) {
			++nSuffixes;
		}

		// create the new subnode
		assert (currentLcp > lastLcp);
		const unsigned int prefixLength = currentLcp - lastLcp;
		const unsigned int currentIndex = index + currentLcp;
		const unsigned int prefixBits = DIM * prefixLength;
		const unsigned int suffixBits = DIM * (WIDTH - (currentIndex + 1));
		Node<DIM>* newSubnode = NodeTypeUtil<DIM>::template buildNodeWithSuffixes<WIDTH>(
				prefixBits, nSuffixes + 1, nSuffixes, suffixBits);
		currentNode->insertAtAddress(currentHcAddress, newSubnode);

		// copy prefix into the new node
		if (prefixLength > 0) {
			const size_t suffixBitsForCopy = suffixBits + DIM;
			MultiDimBitset<DIM>::duplicateHighestBits(suffixToCompareTo_, suffixBitsForCopy,
					prefixLength, newSubnode->getPrefixStartBlock());
		}

		// insert all suffixes into the new subnode
		for (unsigned j = i; j < i + nSuffixes; ++j) {
			const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(buffer_[j], currentIndex, DIM * WIDTH);
			// TODO what if several same addresses in the subnode?
			DynamicNodeOperationsUtil<DIM, WIDTH>::insertSuffix(currentIndex, hcAddress, newSubnode, buffer_[j], tree);
		}

		currentNode = newSubnode;
		currentHcAddress = MultiDimBitset<DIM>::interleaveBits(suffixToCompareTo_, currentIndex + 1, DIM * WIDTH);
	}
}


#endif /* SRC_UTIL_ENTRYBUFFER_H_ */
