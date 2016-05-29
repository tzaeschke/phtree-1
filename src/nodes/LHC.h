/*
 * LHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef LHC_H_
#define LHC_H_

#include <map>
#include <vector>
#include <cstdint>
#include "nodes/TNode.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
class LHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
class LHC: public TNode<DIM, PREF_BLOCKS> {
	friend class LHCIterator<DIM, PREF_BLOCKS, N>;
	friend class AssertionVisitor<DIM>;
	friend class SizeVisitor<DIM>;
public:
	LHC(size_t prefixLength);
	virtual ~LHC();
	NodeIterator<DIM>* begin() const override;
	NodeIterator<DIM>* it(unsigned long hcAddress) const override;
	NodeIterator<DIM>* end() const override;
	void accept(Visitor<DIM>* visitor, size_t depth, unsigned int index) override;
	void recursiveDelete() override;
	size_t getNumberOfContents() const override;
	size_t getMaximumNumberOfContents() const override;
	void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) const override;
	void insertAtAddress(unsigned long hcAddress, const unsigned long* const startSuffixBlock, int id) override;
	void insertAtAddress(unsigned long hcAddress, unsigned long suffix, int id) override;
	void insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) override;
	Node<DIM>* adjustSize() override;

protected:
	string getName() const override;

private:
	static const unsigned long fullBlock = -1;
	static const unsigned int bitsPerBlock = sizeof (unsigned long) * 8;
	// store address, hasSub flag and directlyStored flag
	static const unsigned int addressSubLength = DIM + 2uL;

	// TODO also store flags in reference like in AHC node
	// block : <--------------------------- 64 ---------------------------><--- ...
	// bits  : <-   DIM    -><-  1    |     1          ->|<-   DIM    -><-  1    |     1          -> ...
	// N rows: [ hc address | hasSub? | directlyStored? ] [ hc address | hasSub? | directlyStored? ] ...
	unsigned long addresses_[1 + ((N * (DIM + 2)) - 1) / bitsPerBlock];
	int ids_[N];
	std::uintptr_t references_[N];
	// number of actually filled rows: 0 <= m <= N
	unsigned int m;

	// <found?, index, hasSub?>
	void lookupAddress(unsigned long hcAddress, bool* outExists, unsigned int* outIndex, bool* outHasSub, bool* outDirectlyStored) const;
	void lookupIndex(unsigned int index, unsigned long* outHcAddress, bool* outHasSub, bool* outDirectlyStored) const;
	inline void addRow(unsigned int index, unsigned long hcAddress, bool hasSub, bool directlyStored, std::uintptr_t reference, int id);
	inline void insertAddress(unsigned int index, unsigned long hcAddress);
	inline void insertHasSubFlag(unsigned int index, bool hasSub);
	inline void insertDirectlyStoredFlag(unsigned int index, bool directlyStored);
};

#include <assert.h>
#include <utility>
#include "nodes/AHC.h"
#include "nodes/LHC.h"
#include "iterators/LHCIterator.h"
#include "visitors/Visitor.h"
#include "util/NodeTypeUtil.h"

using namespace std;

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
LHC<DIM, PREF_BLOCKS, N>::LHC(size_t prefixLength) : TNode<DIM, PREF_BLOCKS>(prefixLength),
	addresses_(), ids_(), references_(), m(0) {
	assert (N > 0 && m >= 0 && m <= N);
	assert (N <= (1 << DIM));
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
LHC<DIM, PREF_BLOCKS, N>::~LHC() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::recursiveDelete() {
	bool directlyStored;
	bool hasSub = false;
	unsigned long hcAddress = 0;
	for (unsigned int i = 0; i < m; ++i) {
		lookupIndex(i, &hcAddress, &hasSub, &directlyStored);
		if (hasSub) {
			Node<DIM>* subnode = reinterpret_cast<Node<DIM>*>(references_[i]);
			subnode->recursiveDelete();
		}
	}

	delete this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
string LHC<DIM,PREF_BLOCKS, N>::getName() const {
	return "LHC";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM,PREF_BLOCKS, N>::lookupIndex(unsigned int index, unsigned long* outHcAddress, bool* outHasSub, bool* outDirectlyStored) const {
	assert (index < m && m <= N);
	assert (addressSubLength <= bitsPerBlock);
	assert (outHcAddress && outHasSub);

	const unsigned int firstBit = index * addressSubLength;
	const unsigned int firstFlag = (index + 1) * addressSubLength - 2;
	const unsigned int secondFlag = firstFlag + 1;

	const unsigned int firstBitBlockIndex = firstBit / bitsPerBlock;
	const unsigned int lastBitBlockIndex = secondFlag / bitsPerBlock;
	const unsigned int firstBitIndex = firstBit % bitsPerBlock;
	const unsigned int firstFlagIndex = firstFlag % bitsPerBlock;
	const unsigned int lastBitIndex = secondFlag % bitsPerBlock;

	const unsigned long firstBlock = addresses_[firstBitBlockIndex];
	assert (lastBitBlockIndex - firstBitBlockIndex <= 1);
	if (firstBitBlockIndex == lastBitBlockIndex) {
		// all required bits are in one block
		const unsigned long singleBlockAddressMask = (1uL << DIM) - 1uL;
		const unsigned long extracted = firstBlock >> firstBitIndex;
		(*outHcAddress) = extracted & singleBlockAddressMask;
		(*outHasSub) = (firstBlock >> firstFlagIndex) & 1uL;
		(*outDirectlyStored) = (firstBlock >> lastBitIndex) & 1uL;
	} else if (lastBitIndex < 2) {
		// the address is entirely in one block but the flags are not
		const unsigned long singleBlockAddressMask = (1uL << DIM) - 1uL;
		const unsigned long extracted = firstBlock >> firstBitIndex;
		(*outHcAddress) = extracted & singleBlockAddressMask;

		const unsigned long secondBlock = addresses_[lastBitBlockIndex];
		if (lastBitIndex == 1) {
			// both flags are in the second block
			assert (firstFlagIndex == 0 && lastBitIndex == 1);
			(*outHasSub) = secondBlock & 1uL;
			(*outDirectlyStored) = (secondBlock >> lastBitIndex) & 1uL;
		} else {
			// the first flag is in the first block and the second flag is in the second block
			assert (firstFlagIndex == bitsPerBlock - 1 && lastBitIndex == 0);
			(*outHasSub) = (firstBlock >> firstFlagIndex) & 1uL;
			(*outDirectlyStored) = secondBlock & 1uL;
		}
	} else {
		// the address is split into two blocks and both flags are in the second block
		const unsigned long secondBlock = addresses_[lastBitBlockIndex];
		assert (1 < lastBitIndex && lastBitIndex < addressSubLength - 1);
		const unsigned int firstBlockBits = bitsPerBlock - firstBitIndex;
		const unsigned int secondBlockBits = DIM - firstBlockBits;
		assert (0 < secondBlockBits && secondBlockBits < DIM);
		const unsigned long secondBlockMask = (1uL << secondBlockBits) - 1uL;
		(*outHcAddress) = (firstBlock >> firstBitIndex)
						| ((secondBlock & secondBlockMask) << firstBlockBits);
		assert (*outHcAddress < (1uL << DIM));
		(*outHasSub) = (secondBlock >> firstFlagIndex) & 1uL;
		(*outDirectlyStored) = (secondBlock >> lastBitIndex) & 1uL;
	}

	assert ((!(*outDirectlyStored) || !(*outHasSub)) && "the directly stored flag can only be set if the hasSub flag is false");
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM,PREF_BLOCKS, N>::insertAddress(unsigned int index, unsigned long hcAddress) {
	assert (index < m && m <= N);
	assert (addressSubLength <= bitsPerBlock);

	const unsigned int firstBit = index * addressSubLength;
	const unsigned int lastBit = (index + 1) * addressSubLength - 2;
	const unsigned int firstBlockIndex = firstBit / bitsPerBlock;
	const unsigned int secondBlockIndex = lastBit / bitsPerBlock;
	const unsigned int firstBitIndex = firstBit % bitsPerBlock;
	const unsigned int lastBitIndex = lastBit % bitsPerBlock;
	assert (secondBlockIndex - firstBlockIndex <= 1);
	assert (lastBit - firstBit == DIM);

	const unsigned long firstBlockAddressMask = (1uL << DIM) - 1;
	// [   (   x   )   ]
	addresses_[firstBlockIndex] &= ~(firstBlockAddressMask << firstBitIndex);
	addresses_[firstBlockIndex] |= hcAddress << firstBitIndex;

	if (firstBlockIndex != secondBlockIndex && lastBitIndex != 0) {
		assert (secondBlockIndex - firstBlockIndex == 1);
		// the required bits are in two consecutive blocks
		//       <---------->
		// [     ( x  ] [ y )    ]
		assert (DIM > lastBitIndex);
		const unsigned int firstBlockBits = bitsPerBlock - firstBitIndex;
		const unsigned int secondBlockBits = DIM - firstBlockBits;
		assert (0 < secondBlockBits && secondBlockBits < DIM);
		const unsigned long secondBlockMask = fullBlock << secondBlockBits;
		const unsigned long remainingHcAddress = hcAddress >> firstBlockBits;
		assert (remainingHcAddress < (1uL << secondBlockBits));
		addresses_[secondBlockIndex] &= secondBlockMask;
		addresses_[secondBlockIndex] |= remainingHcAddress;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM,PREF_BLOCKS, N>::insertHasSubFlag(unsigned int index, bool hasSub) {
	assert (index < m && m <= N);

	const unsigned int nBits = index * (DIM + 2) + DIM;
	const unsigned int blockIndex = nBits / bitsPerBlock;
	const unsigned int bitIndex = nBits % bitsPerBlock;

	if (hasSub) {
		addresses_[blockIndex] |= 1uL << bitIndex;
	} else {
		addresses_[blockIndex] &= ~(1uL << bitIndex);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM,PREF_BLOCKS, N>::insertDirectlyStoredFlag(unsigned int index, bool directlyStored) {
	assert (index < m && m <= N);

	// TODO combine with hasSub insertion
	const unsigned int nBits = index * (DIM + 2) + DIM + 1;
	const unsigned int blockIndex = nBits / bitsPerBlock;
	const unsigned int bitIndex = nBits % bitsPerBlock;

	if (directlyStored) {
		addresses_[blockIndex] |= 1uL << bitIndex;
	} else {
		addresses_[blockIndex] &= ~(1uL << bitIndex);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::lookupAddress(unsigned long hcAddress, bool* outExists,
		unsigned int* outIndex, bool* outHasSub, bool* outDirectlyStored) const {
	if (m == 0) {
		(*outExists) = false;
		(*outIndex) = 0;
		return;
	}

	// perform binary search in range [0, m)
	unsigned int l = 0;
	unsigned int r = m;
	unsigned long currentHcAddress = -1;
	while (l < r) {
		// check interval [l, r)
		const unsigned int middle = (l + r) / 2;
		assert (0 <= middle && middle < m);
		lookupIndex(middle, &currentHcAddress, outHasSub, outDirectlyStored);
		if (currentHcAddress < hcAddress) {
			l = middle + 1;
		} else if (currentHcAddress > hcAddress) {
			r = middle;
		} else {
			// found the correct index
			(*outExists) = true;
			(*outIndex) = middle;
			return;
		}
	}

	// did not find the entry so set the position it should have
	assert (l - r == 0);
	(*outExists) = false;
	(*outIndex) = r;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent) const {
	assert (address < 1uL << DIM);

	outContent.address = address;
	unsigned int index = m;
	lookupAddress(address, &outContent.exists, &index, &outContent.hasSubnode, &outContent.directlyStoredSuffix);

	if (outContent.exists) {
		if (outContent.hasSubnode) {
			outContent.subnode = reinterpret_cast<Node<DIM>*>(references_[index]);
		} else {
			if (outContent.directlyStoredSuffix) {
				outContent.suffix = reinterpret_cast<unsigned long>(references_[index]);
			} else {
				// return the stored reference
				outContent.suffixStartBlock = reinterpret_cast<const unsigned long*>(references_[index]);
			}
			outContent.id = ids_[index];
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::addRow(unsigned int index, unsigned long newHcAddress,
		bool newHasSub, bool newDirectlyStored, uintptr_t newReference, int newId) {

	assert (!((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(newHcAddress)).exists);

	++m;
	if (index != m - 1) {
		assert (index < m && m <= N);
		// move all contents as of the given index
		// (copying from lower index because of forward prefetching)
		int lastId = ids_[index];
		uintptr_t lastRef = references_[index];
		unsigned long lastAddress = 0;
		bool lastHasSub = false;
		bool lastDirStored = false;
		lookupIndex(index, &lastAddress, &lastHasSub, &lastDirStored);
		assert (lastAddress > newHcAddress);
		unsigned long tmpAddress = 0;
		bool tmpHasSub = false;
		bool tmpDirStored = false;

		for (unsigned i = index + 1; i < m - 1; ++i) {
			const int tmpId = ids_[i];
			const uintptr_t tmpRef = references_[i];
			lookupIndex(i, &tmpAddress, &tmpHasSub, &tmpDirStored);
			assert (tmpAddress > newHcAddress);
			ids_[i] = lastId;
			references_[i] = lastRef;
			insertAddress(i, lastAddress);
			insertHasSubFlag(i, lastHasSub);
			insertDirectlyStoredFlag(i, lastDirStored);
			lastId = tmpId;
			lastRef = tmpRef;
			lastAddress = tmpAddress;
			lastHasSub = tmpHasSub;
			lastDirStored = tmpDirStored;
		}

		ids_[m - 1] = lastId;
		references_[m - 1] = lastRef;
		insertAddress(m - 1, lastAddress);
		insertHasSubFlag(m - 1, lastHasSub);
		insertDirectlyStoredFlag(m - 1, lastDirStored);
	}

	// insert the new entry at the freed index
	ids_[index] = newId;
	references_[index] = newReference;
	insertAddress(index, newHcAddress);
	insertHasSubFlag(index, newHasSub);
	insertDirectlyStoredFlag(index, newDirectlyStored);

	assert (m <= N);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, const unsigned long* const startSuffixBlock, int id) {
	assert (hcAddress < 1uL << DIM);

	unsigned int index = m;
	bool hasSub = false;
	bool exists = false;
	bool directlyStored = false;
	lookupAddress(hcAddress, &exists, &index, &hasSub, &directlyStored);
	assert (index <= m);
	uintptr_t reference = reinterpret_cast<uintptr_t>(startSuffixBlock);

	if (exists) {
		// replace the contents at the address
		ids_[index] = id;
		references_[index] = reference;
		insertHasSubFlag(index, false);
		insertDirectlyStoredFlag(index, false);
	} else {
		// add a new entry
		assert (m < N && "the maximum number of entries must not have been reached");
		addRow(index, hcAddress, false, false, reference, id);
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).id == id);
	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
	assert (!((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).hasSubnode);
	assert (!((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).directlyStoredSuffix);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, unsigned long suffix, int id) {
	assert (hcAddress < 1uL << DIM);
	assert (suffix < (1uL << (sizeof(uintptr_t) * 8uL - 2uL)));

	unsigned int index = m;
	bool hasSub = false;
	bool exists = false;
	bool directlyStored = false;
	lookupAddress(hcAddress, &exists, &index, &hasSub, &directlyStored);
	assert (index <= m);
	uintptr_t reference = reinterpret_cast<uintptr_t>(suffix);

	if (exists) {
		// replace the contents at the address
		ids_[index] = id;
		references_[index] = reference;
		insertHasSubFlag(index, false);
		insertDirectlyStoredFlag(index, true);
	} else {
		// add a new entry
		assert (m < N && "the maximum number of entries must not have been reached");
		addRow(index, hcAddress, false, true, reference, id);
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).id == id);
	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).directlyStoredSuffix);
	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).suffix == suffix);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) {
	assert (hcAddress < 1uL << DIM);

	unsigned int index = m;
	bool hasSub = false;
	bool exists = false;
	bool directlyStored = false;
	lookupAddress(hcAddress, &exists, &index, &hasSub, &directlyStored);
	assert (index <= m);
	uintptr_t reference = reinterpret_cast<uintptr_t>(subnode);

	if (exists) {
		// replace the contents at the address
		ids_[index] = 0; // TODO not really needed
		references_[index] = reference;
		insertHasSubFlag(index, true);
		insertDirectlyStoredFlag(index, false); // TODO also not really needed
	} else {
		// add a new entry
		assert (m < N && "the maximum number of entries must not have been reached");
		addRow(index, hcAddress, true, false, reference, 0);
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).hasSubnode);
}


template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
Node<DIM>* LHC<DIM, PREF_BLOCKS, N>::adjustSize() {
	// TODO put this method into insert method instead
	if (m <= N) {
		return this;
	} else {
		return NodeTypeUtil<DIM>::copyIntoLargerNode(N + 1, this);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS, N>::begin() const {
	return new LHCIterator<DIM, PREF_BLOCKS, N>(*this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS, N>::it(unsigned long hcAddress) const {
	return new LHCIterator<DIM, PREF_BLOCKS, N>(hcAddress, *this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS, N>::end() const {
	NodeIterator<DIM>* it = new LHCIterator<DIM, PREF_BLOCKS, N>(1uL << DIM, *this);
	++(*it); // TODO needed?
	return it;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
size_t LHC<DIM, PREF_BLOCKS, N>::getNumberOfContents() const {
	return m;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
size_t LHC<DIM, PREF_BLOCKS, N>::getMaximumNumberOfContents() const {
	return N;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::accept(Visitor<DIM>* visitor, size_t depth, unsigned int index) {
	visitor->visit(this, depth, index);
	TNode<DIM, PREF_BLOCKS>::accept(visitor, depth, index);
}

#endif /* LHC_H_ */
