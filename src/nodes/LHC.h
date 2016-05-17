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
#include "nodes/LHCAddressContent.h"
#include "util/MultiDimBitset.h"

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
	NodeIterator<DIM>* begin() override;
	NodeIterator<DIM>* end() override;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) override;
	virtual void recursiveDelete() override;
	virtual size_t getNumberOfContents() const override;
	virtual size_t getMaximumNumberOfContents() const override;
	void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) override;
	void insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) override;
	void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) override;
	Node<DIM>* adjustSize() override;

protected:
	string getName() const override;

private:
	static const unsigned long fullBlock = -1;
	static const unsigned int bitsPerBlock = sizeof (unsigned long) * 8;
	static const unsigned int addressSubLength = DIM + 1;

	// block : <------------------ 64 -------------------><--- ...
	// bits  : <-   DIM    -><-  1  ->|<-   DIM    -><-  1  -> ...
	// N rows: [ hc address | hasSub ] [ hc address | hasSub ] ...
	unsigned long addresses_[1 + ((N * (DIM + 1)) - 1) / bitsPerBlock];
	int ids_[N];
	std::uintptr_t references_[N];
	// number of actually filled rows: 0 <= m <= N
	unsigned int m;

	// <found?, index, hasSub?>
	void lookupAddress(unsigned long hcAddress, bool* outExists, unsigned int* outIndex, bool* outHasSub) const;
	void lookupIndex(unsigned int index, unsigned long* outHcAddress, bool* outHasSub) const;
	inline void addRow(unsigned int index, unsigned long hcAddress, bool hasSub, std::uintptr_t reference, int id);
	inline void insertAddress(unsigned int index, unsigned long hcAddress);
	inline void insertSub(unsigned int index, bool hasSub);
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
	for (unsigned int i = 0; i < m; ++i) {
		bool hasSub = false;
		unsigned long hcAddress = 0;
		lookupIndex(i, &hcAddress, &hasSub);
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
void LHC<DIM,PREF_BLOCKS, N>::lookupIndex(unsigned int index, unsigned long* outHcAddress, bool* outHasSub) const {
	assert (index < m && m <= N);
	assert (addressSubLength <= bitsPerBlock);
	assert (outHcAddress && outHasSub);

	const unsigned int firstBit = index * addressSubLength;
	const unsigned int lastBit = (index + 1) * addressSubLength - 1;
	const unsigned int firstBlockIndex = firstBit / bitsPerBlock;
	const unsigned int secondBlockIndex = lastBit / bitsPerBlock;
	const unsigned int firstBitIndex = firstBit % bitsPerBlock;
	const unsigned int lastBitIndex = lastBit % bitsPerBlock;
	assert (secondBlockIndex - firstBlockIndex <= 1);

	const unsigned long firstBlock = addresses_[firstBlockIndex];
	const unsigned long secondBlock = addresses_[secondBlockIndex];
	if (firstBlockIndex == secondBlockIndex || lastBitIndex == 0) {
		// the required bits are in one block
		//     <------->
		// [   (   x   )   ]
		const unsigned long singleBlockAddressMask = (1uL << DIM) - 1;
		const unsigned long extracted = firstBlock >> firstBitIndex;
		(*outHcAddress) = extracted & singleBlockAddressMask;
	} else {
		// the required bits are in two consecutive blocks
		//       <---------->
		// [     ( x  ] [ y )    ]
		assert (DIM > lastBitIndex);
		const unsigned int firstBlockBits = bitsPerBlock - firstBitIndex;
		const unsigned int secondBlockBits = DIM - firstBlockBits;
		assert (0 < secondBlockBits && secondBlockBits < DIM);
		const unsigned long secondBlockMask = (1 << secondBlockBits) - 1;
		(*outHcAddress) = (firstBlock >> firstBitIndex)
				| ((secondBlock & secondBlockMask) << firstBlockBits);
	}

	(*outHasSub) = (secondBlock >> lastBitIndex) & 1uL;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM,PREF_BLOCKS, N>::insertAddress(unsigned int index, unsigned long hcAddress) {
	assert (index < m && m <= N);
	assert (addressSubLength <= bitsPerBlock);

	const unsigned int firstBit = index * (DIM + 1);
	const unsigned int lastBit = (index + 1) * (DIM + 1) - 1;
	const unsigned int firstBlockIndex = firstBit / bitsPerBlock;
	const unsigned int secondBlockIndex = lastBit / bitsPerBlock;
	const unsigned int firstBitIndex = firstBit % bitsPerBlock;
	const unsigned int lastBitIndex = lastBit % bitsPerBlock;
	assert (secondBlockIndex - firstBlockIndex <= 1);

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
void LHC<DIM,PREF_BLOCKS, N>::insertSub(unsigned int index, bool hasSub) {
	assert (index < m && m <= N);

	const unsigned int nBits = index * (DIM + 1) + DIM;
	const unsigned int blockIndex = nBits / bitsPerBlock;
	const unsigned int bitIndex = nBits % bitsPerBlock;

	if (hasSub) {
		addresses_[blockIndex] |= 1uL << bitIndex;
	} else {
		addresses_[blockIndex] &= ~(1uL << bitIndex);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::lookupAddress(unsigned long hcAddress, bool* outExists, unsigned int* outIndex, bool* outHasSub) const {
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
		lookupIndex(middle, &currentHcAddress, outHasSub);
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
void LHC<DIM, PREF_BLOCKS, N>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent) {
	assert (address < 1uL << DIM);

	outContent.address = address;
	unsigned int index = m;
	lookupAddress(address, &outContent.exists, &index, &outContent.hasSubnode);

	if (outContent.exists) {
		if (outContent.hasSubnode) {
			outContent.subnode = reinterpret_cast<Node<DIM>*>(references_[index]);
		} else {
			outContent.suffixStartBlock = reinterpret_cast<unsigned long*>(references_[index]);
			outContent.id = ids_[index];
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::addRow(unsigned int index, unsigned long newHcAddress,
		bool newHasSub, uintptr_t newReference, int newId) {

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
		lookupIndex(index, &lastAddress, &lastHasSub);
		assert (lastAddress > newHcAddress);
		unsigned long tmpAddress = 0;
		bool tmpHasSub = false;

		for (unsigned i = index + 1; i < m - 1; ++i) {
			const int tmpId = ids_[i];
			const uintptr_t tmpRef = references_[i];
			lookupIndex(i, &tmpAddress, &tmpHasSub);
			assert (tmpAddress > newHcAddress);
			ids_[i] = lastId;
			references_[i] = lastRef;
			insertAddress(i, lastAddress);
			insertSub(i, lastHasSub);
			lastId = tmpId;
			lastRef = tmpRef;
			lastAddress = tmpAddress;
			lastHasSub = tmpHasSub;
		}

		ids_[m - 1] = lastId;
		references_[m - 1] = lastRef;
		insertAddress(m - 1, lastAddress);
		insertSub(m - 1, lastHasSub);
	}

	// insert the new entry at the freed index
	ids_[index] = newId;
	references_[index] = newReference;
	insertAddress(index, newHcAddress);
	insertSub(index, newHasSub);

	assert (m <= N);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) {
	assert (hcAddress < 1uL << DIM);

	unsigned int index = m;
	bool hasSub = false;
	bool exists = false;
	lookupAddress(hcAddress, &exists, &index, &hasSub);
	assert (index <= m);
	uintptr_t reference = reinterpret_cast<uintptr_t>(startSuffixBlock);;

	if (exists) {
		// replace the contents at the address
		ids_[index] = id;
		references_[index] = reference;
		insertSub(index, false);
	} else {
		// add a new entry
		assert (m < N && "the maximum number of entries must not have been reached");
		addRow(index, hcAddress, false, reference, id);
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void LHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) {
	assert (hcAddress < 1uL << DIM);

	unsigned int index = m;
	bool hasSub = false;
	bool exists = false;
	lookupAddress(hcAddress, &exists, &index, &hasSub);
	assert (index <= m);
	uintptr_t reference = reinterpret_cast<uintptr_t>(subnode);

	if (exists) {
		// replace the contents at the address
		ids_[index] = 0; // TODO not really needed
		references_[index] = reference;
		insertSub(index, true);
	} else {
		// add a new entry
		assert (m < N && "the maximum number of entries must not have been reached");
		addRow(index, hcAddress, true, reference, 0);
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
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
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS, N>::begin() {
	return new LHCIterator<DIM, PREF_BLOCKS, N>(*this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS, N>::end() {
	return new LHCIterator<DIM, PREF_BLOCKS, N>(1uL << DIM, *this);
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
void LHC<DIM, PREF_BLOCKS, N>::accept(Visitor<DIM>* visitor, size_t depth) {
	visitor->visit(this, depth);
	TNode<DIM, PREF_BLOCKS>::accept(visitor, depth);
}

#endif /* LHC_H_ */
