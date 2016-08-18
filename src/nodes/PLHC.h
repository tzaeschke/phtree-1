#ifndef PLHC_H_
#define PLHC_H_

#include <map>
#include <vector>
#include <cstdint>
#include <atomic>
#include "nodes/TNode.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
class PLHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM>
class NodeTypeUtil;

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
class PLHC: public TNode<DIM, PREF_BLOCKS> {
	friend class PLHCIterator<DIM, PREF_BLOCKS, N>;
	friend class AssertionVisitor<DIM>;
	friend class SizeVisitor<DIM>;
public:
	explicit PLHC(size_t prefixLength);
	virtual ~PLHC();
	NodeIterator<DIM>* begin() const override;
	NodeIterator<DIM>* it(unsigned long hcAddress) const override;
	NodeIterator<DIM>* end() const override;
	void accept(Visitor<DIM>* visitor, size_t depth, unsigned int index) override;
	void recursiveDelete() override;
	bool full() const override;
	size_t getNumberOfContents() const override;
	size_t getMaximumNumberOfContents() const override;
	void lookup(unsigned long address, NodeAddressContent<DIM>& outContent, bool resolveSuffixIndex) const override;
	bool insertAtAddress(unsigned long hcAddress, uintptr_t pointer) override;
	bool insertAtAddress(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) override;
	bool insertAtAddress(unsigned long hcAddress, unsigned long suffix, int id) override;
	bool insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) override;
	void linearCopyFromOther(unsigned long hcAddress, uintptr_t pointer) override;
	void linearCopyFromOther(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) override;
	void linearCopyFromOther(unsigned long hcAddress, unsigned long suffix, int id) override;
	void linearCopyFromOther(unsigned long hcAddress, const Node<DIM>* const subnode) override;
	bool updateAddress(uintptr_t pointer, const NodeAddressContent<DIM>& prevContent) override;
	bool updateAddress(const Node<DIM>* const subnode, const NodeAddressContent<DIM>& prevContent) override;
	bool updateAddressToSpinlock(const NodeAddressContent<DIM>& prevContent) override;
	void updateAddressFromSpinlock(unsigned long hcAddress, const Node<DIM>* const subnode) override;
	void updateAddressFromSpinlock(unsigned long hcAddress, uintptr_t pointer) override;
	string getName() const override;
	NodeType getType() const override { return AtomicLinear; }

private:
	static const unsigned long fullBlock = -1;
	static const unsigned int bitsPerBlock = sizeof (unsigned long) * 8;
	static const unsigned long KEY_INVALID = -1uL;
	// the spinlock is handled as a special pointer but since pointers usually have
	// the three least significant bits unset the spinlock is uniquely distinguishable
	static const uintptr_t REF_SPINLOCK = 4;

	// map of valid HC addresses in ascending order
	// block : <-------------------- 64 --------------------><--- ...
	// bits  : <-   DIM    ->|<-   DIM    -> * N
	// N rows: [ hc address ] [ hc address ] ...
	unsigned long orderedAddresses_[1 + ((N * DIM) - 1) / bitsPerBlock];
	std::atomic<unsigned long> unorderedAddresses_[N];
	// stores flags in 2 lowest bits per reference:
	// meaning of flags: isPointer | isSuffix
	// 00 - special pointer
	// 01 - the entry directly stores a suffix and the ID
	// 10 - the entry holds a reference to a subnode
	// 11 - the entry holds the index of the suffix and the ID
	std::atomic<std::uintptr_t> orderedReferences_[N];
	std::atomic<std::uintptr_t> unorderedReferences_[N];

	// number of actually filled rows: 0 <= m <= N
	unsigned int mSorted;
	std::atomic<unsigned int> mUnsorted;

	bool update(size_t hcAddress, uintptr_t oldReference, uintptr_t newReference);
	bool updateSorted(size_t sortedIndex, uintptr_t oldReference, uintptr_t newReference);								// OK
	bool updateUnsorted(size_t hcAddress, uintptr_t oldReference, uintptr_t newReference);								// OK
	void lookupAddressSorted(unsigned long hcAddress, bool* outExists, unsigned int* outIndex) const;
	void lookupIndexSorted(unsigned int index, unsigned long* outHcAddress) const;
	void lookupAddressUnsorted(unsigned long hcAddress, bool* outExists, unsigned int* outIndex) const;
	void lookupIndexUnsorted(unsigned int index, unsigned long* outHcAddress) const;

	void insertSorted(unsigned long hcAddres, uintptr_t reference);
	inline void lookupSorted(unsigned long hcAddress, bool* exists, uintptr_t* ref) const;
	inline void lookupUnsorted(unsigned long hcAddress, bool* exists, uintptr_t* ref) const;

	size_t findUnsortedInsertIndex(size_t hcAddress);																// OK

	void fillLookupContent(NodeAddressContent<DIM>& outContent, uintptr_t reference, bool resolveSuffixIndex) const;
	inline void interpretReference(std::uintptr_t ref, bool* isPointer, bool* isSuffix) const;
	inline uintptr_t reconstructReference(const NodeAddressContent<DIM>& prevContent) const;
};

#include <assert.h>
#include <utility>
#include <stdexcept>
#include "iterators/PLHCIterator.h"
#include "visitors/Visitor.h"
#include "util/NodeTypeUtil.h"

using namespace std;
template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
PLHC<DIM, PREF_BLOCKS, N>::PLHC(size_t prefixLength) : TNode<DIM, PREF_BLOCKS>(prefixLength),
	orderedAddresses_(), orderedReferences_(), mSorted(0), mUnsorted(0) {
	assert (N <= (1 << DIM));

	for (unsigned i = 0; i < N; ++i) {
		unorderedAddresses_[i] = KEY_INVALID;
		unorderedReferences_[i] = REF_SPINLOCK;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
PLHC<DIM, PREF_BLOCKS, N>::~PLHC() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::recursiveDelete() {
	// The PLHC node is supposed to work as an intermediate node and should not be contained in the final version of a tree.
	// Therefore, there is no need to recursively delete.
	throw runtime_error("recursive delete unsupported");
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
string PLHC<DIM,PREF_BLOCKS,N>::getName() const {
	return "PLHC<" + to_string(DIM) + "," + to_string(PREF_BLOCKS) + "," + to_string(N) + ">";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM,PREF_BLOCKS, N>::lookupIndexSorted(unsigned int index, unsigned long* outHcAddress) const {
	assert (index < mSorted && mSorted <= N);
	assert (DIM <= bitsPerBlock);

	const unsigned int firstBit = index * DIM;
	const unsigned int lastBit = (index + 1) * DIM;
	const unsigned int firstBitBlockIndex = firstBit / bitsPerBlock;
	const unsigned int lastBitBlockIndex = lastBit / bitsPerBlock;
	const unsigned int firstBitIndex = firstBit % bitsPerBlock;
	const unsigned int lastBitIndex = lastBit % bitsPerBlock;

	const unsigned long firstBlock = orderedAddresses_[firstBitBlockIndex];
	assert (lastBitBlockIndex - firstBitBlockIndex <= 1);
	if (firstBitBlockIndex == lastBitBlockIndex) {
		// all required bits are in one block
		const unsigned long singleBlockAddressMask = (1uL << DIM) - 1uL;
		const unsigned long extracted = firstBlock >> firstBitIndex;
		(*outHcAddress) = extracted & singleBlockAddressMask;
	} else {
		// the address is split into two blocks and both flags are in the second block
		const unsigned long secondBlock = orderedAddresses_[lastBitBlockIndex];
		assert (1 < lastBitIndex && lastBitIndex < DIM);
		const unsigned int firstBlockBits = bitsPerBlock - firstBitIndex;
		const unsigned int secondBlockBits = DIM - firstBlockBits;
		assert (0 < secondBlockBits && secondBlockBits < DIM);
		const unsigned long secondBlockMask = (1uL << secondBlockBits) - 1uL;
		(*outHcAddress) = (firstBlock >> firstBitIndex)
						| ((secondBlock & secondBlockMask) << firstBlockBits);
	}

	assert (*outHcAddress < (1uL << DIM));
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
size_t PLHC<DIM, PREF_BLOCKS, N>::findUnsortedInsertIndex(size_t hcAddress) {
	const size_t mUnsortedSnap = mUnsorted;
	// validate if the key was already inserted
	for (unsigned i = 0; i < mUnsortedSnap; ++i) {
		if (unorderedReferences_[i] == hcAddress) {
			return -1uL;
		}
	}

	// try to find an unused slot to insert the entry
	for (unsigned i = mUnsortedSnap; i < N; ++i) {
		uintptr_t expectedUnused = KEY_INVALID;
		if (atomic_compare_exchange_strong(&unorderedAddresses_[i], &expectedUnused, hcAddress)) {
			// successfully found a new slot that can be used for insertion
			++mUnsorted;
			return i;
		} else if (expectedUnused == hcAddress) {
			// another thread simultaneously inserted the same address so fail
			break;
		}
	}

	return -1;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::updateSorted(size_t sortedIndex, uintptr_t oldReference, uintptr_t newReference) {
	return atomic_compare_exchange_strong(&orderedReferences_[sortedIndex], &oldReference, newReference);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::updateUnsorted(size_t hcAddress, uintptr_t oldReference, uintptr_t newReference) {
	const size_t mUnsortedSnap = mUnsorted;
	for (unsigned i = 0; i < mUnsortedSnap; ++i) {
		if (unorderedAddresses_[i] == hcAddress) {
			return atomic_compare_exchange_strong(&unorderedReferences_[i], &oldReference, newReference);
		}
	}

	assert (false);
	return false;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::lookupAddressUnsorted(unsigned long hcAddress, bool* outExists, unsigned int* outIndex) const {
	const size_t mUnsortedSnap = mUnsorted;
	for (unsigned i = 0; i < mUnsortedSnap; ++i) {
		if (unorderedAddresses_[i] == hcAddress) {
			(*outExists) = true;
			(*outIndex) = i;
			return;
		}
	}

	(*outExists) = false;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::lookupIndexUnsorted(unsigned int index, unsigned long* outHcAddress) const {
	assert (index < mUnsorted && mUnsorted <= N);
	assert (unorderedAddresses_[index] != KEY_INVALID);
	while (unorderedReferences_[index] == REF_SPINLOCK) { }
	(*outHcAddress) = unorderedAddresses_[index];
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::lookupAddressSorted(unsigned long hcAddress, bool* outExists,
		unsigned int* outIndex) const {
	if (mSorted == 0) {
		(*outExists) = false;
		(*outIndex) = 0;
		return;
	}

	// perform binary search in range [0, m)
	unsigned int l = 0;
	unsigned int r = mSorted;
	unsigned long currentHcAddress = -1;
	while (l < r) {
		// check interval [l, r)
		const unsigned int middle = (l + r) / 2;
		assert (0 <= middle && middle < mSorted);
		lookupIndexSorted(middle, &currentHcAddress);
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

	assert (l - r == 0);
	(*outExists) = false;
	(*outIndex) = r;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::interpretReference(std::uintptr_t ref, bool* isPointer, bool* isSuffix) const {
	(*isSuffix) = ref & 1uL;
	(*isPointer) = (ref >> 1uL) & 1uL;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
inline uintptr_t PLHC<DIM, PREF_BLOCKS, N>::reconstructReference(const NodeAddressContent<DIM>& prevContent) const {
	assert (prevContent.exists);
	uintptr_t ref;
	if (prevContent.hasSpecialPointer) {
		ref = prevContent.specialPointer;
	} else if (prevContent.hasSubnode) {
		const uintptr_t subRef = reinterpret_cast<uintptr_t>(prevContent.subnode);
		ref = reinterpret_cast<uintptr_t>(subRef | 2);
	} else if (prevContent.directlyStoredSuffix) {
		const unsigned long upperId = prevContent.id;
		ref = reinterpret_cast<uintptr_t>((upperId << 32) | (prevContent.suffix << 2) | 1);
	} else {
		const unsigned long upperId = prevContent.id;
		const unsigned long index = this->getSuffixStorage()->getIndexFromPointer(prevContent.suffixStartBlock);
		const unsigned long suffixStartBlockIndexExtended = (upperId << 32) | (index << 2) | 3;
		ref = reinterpret_cast<uintptr_t>(suffixStartBlockIndexExtended);
	}

	return ref;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::fillLookupContent(NodeAddressContent<DIM>& outContent,
		uintptr_t reference, bool resolveSuffixIndex) const {
	assert (outContent.exists);
	bool isPointer, isSuffix;
	interpretReference(reference, &isPointer, &isSuffix);
	outContent.hasSubnode = isPointer && !isSuffix;
	outContent.directlyStoredSuffix = !isPointer && isSuffix;
	outContent.hasSpecialPointer = !isPointer && !isSuffix;

	if (outContent.exists) {
		const unsigned long flagMask = ~(3uL);
		if (outContent.hasSubnode) {
			outContent.subnode = reinterpret_cast<Node<DIM>*>(reference & flagMask);
		} else if (outContent.hasSpecialPointer) {
			assert ((reference & flagMask) == reference);
			outContent.specialPointer = reference;
		} else {
			const unsigned long suffixAndId = reinterpret_cast<unsigned long>(reference);
			const unsigned long suffixMask = (-1uL) >> 32;
			const unsigned int suffixPart = (suffixAndId & suffixMask) >> 2;
			if (outContent.directlyStoredSuffix) {
				outContent.suffix = suffixPart;
			} else {
				if (resolveSuffixIndex) {
					outContent.suffixStartBlock = this->getSuffixStartBlockPointerFromIndex(suffixPart);
				} else {
					outContent.suffixStartBlockIndex = suffixPart;
				}
			}

			const unsigned long idPart = (suffixAndId & (~suffixMask)) >> 32;
			assert (idPart < (1uL << 32));
			outContent.id = idPart;
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent, bool resolveSuffixIndex) const {
	assert (address < 1uL << DIM);
	uintptr_t ref;
	outContent.address = address;
	lookupSorted(address, &outContent.exists, &ref);
	if (!outContent.exists) {
		lookupUnsorted(address, &outContent.exists, &ref);
	}

	if (outContent.exists) {
		fillLookupContent(outContent, ref, resolveSuffixIndex);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
inline void PLHC<DIM, PREF_BLOCKS, N>::lookupSorted(unsigned long hcAddress, bool* exists, uintptr_t* ref) const {
	unsigned int index = N;
	lookupAddressSorted(hcAddress, exists, &index);
	if (*exists) {
		do {
		*ref = orderedReferences_[index];
		} while ((*ref) == REF_SPINLOCK);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
inline void PLHC<DIM, PREF_BLOCKS, N>::lookupUnsorted(unsigned long hcAddress, bool* exists, uintptr_t* ref) const {
	unsigned int index = N;
	lookupAddressUnsorted(hcAddress, exists, &index);
	if (*exists) {
		do {
			(*ref) = unorderedReferences_[index];
		} while ((*ref) == REF_SPINLOCK);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, uintptr_t pointer) {
	assert (hcAddress < 1uL << DIM);
	assert ((pointer & 3) == 0);
	// format: [ pointer (62) | flags - 00 (2) ]
	size_t insertIndex = findUnsortedInsertIndex(hcAddress);
	if (insertIndex != (-1uL)) {
		unorderedReferences_[insertIndex] = pointer;
	}

	return insertIndex != (-1uL);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) {
	assert (hcAddress < 1uL << DIM);
	assert (suffixStartBlockIndex < (1uL << 30));
	// format: [ ID (32) | suffix index (30) | flags - 11 (2) ]

	size_t insertIndex = findUnsortedInsertIndex(hcAddress);
	if (insertIndex != (-1uL)) {
		const unsigned long upperId = id;
		const unsigned long suffixStartBlockIndexExtended = (upperId << 32) | (suffixStartBlockIndex << 2) | 3;
		const uintptr_t reference = reinterpret_cast<uintptr_t>(suffixStartBlockIndexExtended);
		unorderedReferences_[insertIndex] = reference;
	}

	return insertIndex != (-1uL);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, unsigned long suffix, int id) {
	assert (hcAddress < 1uL << DIM);
	assert (suffix < (1uL << 30));
	// format: [ ID (32) | suffix (30) | flags - 01 (2) ]

	size_t insertIndex = findUnsortedInsertIndex(hcAddress);
	if (insertIndex != (-1uL)) {
		const unsigned long upperId = id;
		const uintptr_t reference = reinterpret_cast<uintptr_t>((upperId << 32) | (suffix << 2) | 1);
		unorderedReferences_[insertIndex] = reference;
	}

	return insertIndex != (-1uL);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) {
	assert (hcAddress < 1uL << DIM);
	// format: [ subnode reference (62) | flags - 10 (2) ]

	size_t insertIndex = findUnsortedInsertIndex(hcAddress);
	if (insertIndex != (-1uL)) {
		const uintptr_t subRef = reinterpret_cast<uintptr_t>(subnode);
		assert ((subRef & 3) == 0);
		const uintptr_t reference = reinterpret_cast<uintptr_t>(subRef | 2);
		unorderedReferences_[insertIndex] = reference;
	}

	return insertIndex != (-1uL);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::update(size_t hcAddress, uintptr_t oldReference, uintptr_t newReference) {
	bool exists;
	unsigned int index;
	lookupAddressSorted(hcAddress, &exists, &index);
	if (exists) {
		// update in the sorted part
		return updateSorted(index, oldReference, newReference);
	} else {
		// update in the unsorted part
		return updateUnsorted(hcAddress, oldReference, newReference);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::updateAddress(uintptr_t pointer, const NodeAddressContent<DIM>& prevContent) {
	uintptr_t oldRef = reconstructReference(prevContent);
	return update(prevContent.address, oldRef, pointer);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::updateAddress(const Node<DIM>* const subnode, const NodeAddressContent<DIM>& prevContent) {
	uintptr_t oldRef = reconstructReference(prevContent);
	const uintptr_t subRef = reinterpret_cast<uintptr_t>(subnode);
	assert ((subRef & 3) == 0);
	const uintptr_t reference = reinterpret_cast<uintptr_t>(subRef | 2);
	return update(prevContent.address, oldRef, reference);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::updateAddressToSpinlock(const NodeAddressContent<DIM>& prevContent) {
	uintptr_t oldRef = reconstructReference(prevContent);
	return update(prevContent.address, oldRef, REF_SPINLOCK);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::updateAddressFromSpinlock(unsigned long hcAddress, const Node<DIM>* const subnode) {
	const uintptr_t subRef = reinterpret_cast<uintptr_t>(subnode);
	assert ((subRef & 3) == 0);
	const uintptr_t reference = reinterpret_cast<uintptr_t>(subRef | 2);
	const bool success = update(hcAddress, REF_SPINLOCK, reference);
	assert (success);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::updateAddressFromSpinlock(unsigned long hcAddress, uintptr_t pointer) {
	const bool success = update(hcAddress, REF_SPINLOCK, pointer);
	assert (success);
}


template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::linearCopyFromOther(unsigned long hcAddress, uintptr_t pointer) {
	insertSorted(hcAddress, pointer);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::linearCopyFromOther(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) {
	const unsigned long upperId = id;
	const unsigned long suffixStartBlockIndexExtended = (upperId << 32) | (suffixStartBlockIndex << 2) | 3;
	const uintptr_t reference = reinterpret_cast<uintptr_t>(suffixStartBlockIndexExtended);
	insertSorted(hcAddress, reference);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::linearCopyFromOther(unsigned long hcAddress, unsigned long suffix, int id) {
	const unsigned long upperId = id;
	const uintptr_t reference = reinterpret_cast<uintptr_t>((upperId << 32) | (suffix << 2) | 1);
	insertSorted(hcAddress, reference);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::linearCopyFromOther(unsigned long hcAddress, const Node<DIM>* const subnode) {
	const uintptr_t subRef = reinterpret_cast<uintptr_t>(subnode);
	assert ((subRef & 3) == 0);
	const uintptr_t reference = reinterpret_cast<uintptr_t>(subRef | 2);
	insertSorted(hcAddress, reference);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::insertSorted(unsigned long hcAddress, uintptr_t reference) {
	const unsigned int firstBit = mSorted * DIM;
	const unsigned int lastBit = (mSorted + 1) * DIM;
	const unsigned int firstBlockIndex = firstBit / bitsPerBlock;
	const unsigned int secondBlockIndex = lastBit / bitsPerBlock;
	const unsigned int firstBitIndex = firstBit % bitsPerBlock;
	const unsigned int lastBitIndex = lastBit % bitsPerBlock;
	assert (secondBlockIndex - firstBlockIndex <= 1);
	assert (lastBit - firstBit == DIM);

	const unsigned long firstBlockAddressMask = (1uL << DIM) - 1;
	// [   (   x   )   ]
	orderedAddresses_[firstBlockIndex] &= ~(firstBlockAddressMask << firstBitIndex);
	orderedAddresses_[firstBlockIndex] |= hcAddress << firstBitIndex;

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
		orderedAddresses_[secondBlockIndex] &= secondBlockMask;
		orderedAddresses_[secondBlockIndex] |= remainingHcAddress;
	}

	orderedReferences_[mSorted] = reference;
	++mSorted;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
bool PLHC<DIM, PREF_BLOCKS, N>::full() const {
	return mUnsorted >= N;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* PLHC<DIM, PREF_BLOCKS, N>::begin() const {
	PLHCIterator<DIM, PREF_BLOCKS, N>* it = new PLHCIterator<DIM, PREF_BLOCKS, N>(*this);
	if (mSorted + mUnsorted == 0) it->setToEnd();
	else it->setToBegin();
	return it;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* PLHC<DIM, PREF_BLOCKS, N>::it(unsigned long hcAddress) const {
	throw runtime_error("can only do full iterations!");
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
NodeIterator<DIM>* PLHC<DIM, PREF_BLOCKS, N>::end() const {
	NodeIterator<DIM>* it = new PLHCIterator<DIM, PREF_BLOCKS, N>(*this);
	it->setToEnd();
	return it;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
size_t PLHC<DIM, PREF_BLOCKS, N>::getNumberOfContents() const {
	return mUnsorted + mSorted;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
size_t PLHC<DIM, PREF_BLOCKS, N>::getMaximumNumberOfContents() const {
	return N + N;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS, unsigned int N>
void PLHC<DIM, PREF_BLOCKS, N>::accept(Visitor<DIM>* visitor, size_t depth, unsigned int index) {
// TODO	visitor->visit(this, depth, index);
	TNode<DIM, PREF_BLOCKS>::accept(visitor, depth, index);
}

#endif /* PLHC_H_ */
