/*
 * AHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_AHC_H_
#define SRC_AHC_H_

#include <vector>
#include <iostream>
#include <cstdint>
#include <atomic>
#include "nodes/TNode.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class AHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class AHC: public TNode<DIM, PREF_BLOCKS> {
	friend class AHCIterator<DIM, PREF_BLOCKS>;
	friend class AssertionVisitor<DIM>;
	friend class SizeVisitor<DIM>;
public:
	explicit AHC(size_t prefixLength);
	explicit AHC(TNode<DIM, PREF_BLOCKS>* node);
	virtual ~AHC();
	NodeIterator<DIM>* begin() const override;
	NodeIterator<DIM>* it(unsigned long hcAddress) const override;
	NodeIterator<DIM>* end() const override;
	void accept(Visitor<DIM>* visitor, size_t depth, unsigned int index) override;
	void recursiveDelete() override;
	size_t getNumberOfContents() const override;
	size_t getMaximumNumberOfContents() const override;
	void lookup(unsigned long address, NodeAddressContent<DIM>& outContent, bool resolveSuffixIndex) const override;
	bool insertAtAddress(unsigned long hcAddress, uintptr_t pointer) override;
	bool insertAtAddress(unsigned long hcAddress, unsigned long suffix, int id) override;
	bool insertAtAddress(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) override;
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
	NodeType getType() const override { return Array; }

private:

	// Spinlock: [ 0...01 | 00 ] => special pointer, invalid reference
	static const uintptr_t REF_SPINLOCK = 4;
	static const uintptr_t REF_EMPTY = 0;

	std::atomic<size_t> nContents;

	// stores flags in 2 lowest bits per reference: isPointer | isSuffix
	// 00 & reference == 0	- entry does not exist
	// 00 & reference != 0	- entry is a special pointer
	// 01 					- the entry directly stores a suffix and the ID
	// 10 					- the entry holds a reference to a subnode
	// 11 					- the entry holds the index of the suffix and the ID
	std::atomic<std::uintptr_t> references_[1 << DIM];
	static const unsigned long refMask = (-1uL) << 2uL; // mask to remove the 2 flag bits

	inline void getRef(unsigned long hcAddress, bool* exists, bool* spinlock, bool* hasSub,
			bool* directlyStoredSuffix, bool* isSpecial, std::uintptr_t* ref) const;
	inline bool atomicUpdate(unsigned long hcAddress, uintptr_t oldRef, uintptr_t newRef);
	inline uintptr_t reconstructReference(const NodeAddressContent<DIM>& prevContent) const;
};

#include <assert.h>
#include "nodes/AHC.h"
#include "iterators/AHCIterator.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "visitors/Visitor.h"

using namespace std;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::AHC(size_t prefixLength) : TNode<DIM, PREF_BLOCKS>(prefixLength), nContents(0), references_() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::AHC(TNode<DIM, PREF_BLOCKS>* other) : TNode<DIM, PREF_BLOCKS>(other), nContents(0), references_() {

	// TODO use more efficient way to convert LHC->AHC
	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = other->end();
	for (it = other->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			insertAtAddress(content.address, content.subnode);
		} else if (content.directlyStoredSuffix) {
			assert (content.suffix < sizeof (int) * 8 - 2);
			insertAtAddress(content.address, content.suffix, content.id);
		} else {
			assert (content.suffixStartBlock);
			insertAtAddress(content.address, content.suffixStartBlock, content.id);
		}
	}

	delete it;
	delete endIt;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::~AHC() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::recursiveDelete() {
	bool hasSubnode, filled, isDirectlyStoredSuffix, isSpecial, isSpinlock;
	uintptr_t ref = 0;
	for (size_t i = 0; i < 1uL << DIM; ++i) {
		do {
			getRef(i, &filled, &isSpinlock, &hasSubnode, &isDirectlyStoredSuffix, &isSpecial, &ref);
		} while (isSpinlock);

		assert (!isSpecial);
		if (filled && hasSubnode) {
			Node<DIM>* subnode = reinterpret_cast<Node<DIM>*>(ref);
			assert (subnode);
			subnode->recursiveDelete();
		}
	}

	if (this->suffixes_) { delete this->suffixes_; }
	delete this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
string AHC<DIM,PREF_BLOCKS>::getName() const {
	return "AHC<" + to_string(DIM) + "," + to_string(PREF_BLOCKS) + ">";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM,PREF_BLOCKS>::getRef(unsigned long hcAddress, bool* exists, bool* spinlock, bool* hasSub,
		bool* directlyStoredSuffix, bool* isSpecial, std::uintptr_t* ref) const {
	assert (hcAddress < 1 << DIM);
	assert (exists && hasSub && ref);

	const uintptr_t reference = references_[hcAddress];
	const bool isSuffix = reference & 1;
	const bool isPointer = (reference >> 1) & 1;

	(*ref) = reference & refMask;
	(*exists) = reference != 0;
	assert ((*exists) || reference == 0);
	(*hasSub) = isPointer && !isSuffix;
	(*directlyStoredSuffix) = !isPointer && isSuffix;
	(*isSpecial) = !isSuffix && !isPointer && (reference != 0);
	(*spinlock) = reference == REF_SPINLOCK;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t AHC<DIM, PREF_BLOCKS>::getNumberOfContents() const {
	return nContents;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t AHC<DIM, PREF_BLOCKS>::getMaximumNumberOfContents() const {
	return 1uL << DIM;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent, bool resolveSuffixIndex) const {
	assert (address < 1uL << DIM);

	uintptr_t ref;
	outContent.address = address;
	bool isSpinlock;
	do {
		getRef(address, &outContent.exists, &isSpinlock, &outContent.hasSubnode,
				&outContent.directlyStoredSuffix, &outContent.hasSpecialPointer, &ref);
	} while (isSpinlock);

	if (outContent.exists) {
		if (outContent.hasSubnode) {
			outContent.subnode = reinterpret_cast<Node<DIM>*>(ref);
		} else if (outContent.hasSpecialPointer) {
			outContent.specialPointer = ref;
		} else {
			const unsigned long suffixAndId = reinterpret_cast<unsigned long>(ref);
			const unsigned long suffixMask = (-1uL) >> 32;
			// convert the stored suffix and shift back where meta flags were stored
			const unsigned int suffixPart = (suffixAndId & suffixMask) >> 2;
			const unsigned long idPart = (suffixAndId & (~suffixMask)) >> 32;
			assert (idPart < (1uL << 32));
			outContent.id = idPart;

			if (outContent.directlyStoredSuffix) {
				outContent.suffix = suffixPart;
			} else {
				if (resolveSuffixIndex) {
					outContent.suffixStartBlock = this->getSuffixStartBlockPointerFromIndex(suffixPart);
				} else {
					outContent.suffixStartBlockIndex = suffixPart;
				}
			}
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, unsigned long  suffix, int id) {
	assert (hcAddress < 1ul << DIM);
	assert (sizeof (unsigned long) == sizeof (uintptr_t));
	assert (suffix < (1uL << (sizeof(int) * 8uL - 2uL)));

	// layout: [ ID (32) | suffix bits (30) | flags (2)]
	// 01 -> internally stored suffix
	// need to shift the suffix in order to have enough space for the meta flags
	const unsigned long extendedId = id;
	const unsigned long suffixShifted = 1 | (suffix << 2) | (extendedId << 32);
	const uintptr_t newRef = reinterpret_cast<uintptr_t>(suffixShifted);
	const bool success = atomicUpdate(hcAddress, REF_EMPTY, newRef);
	if (success) {
		++nContents;
	}

	return success;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) {
	assert (hcAddress < 1ul << DIM);
	assert (suffixStartBlockIndex < (1uL << (8 * sizeof (int) - 2)));

	const unsigned long suffixStartBlockIndexExtended = suffixStartBlockIndex;
	const unsigned long extendedId = id;
	const uintptr_t storedRef = reinterpret_cast<uintptr_t>(suffixStartBlockIndexExtended << 2);
	assert (((storedRef & 3) == 0) && "last 2 bits must not be set!");
	// layout: [ ID (32) | suffix index (30) | flags (2)]
	// 11 -> pointer to suffix
	const uintptr_t newRef = 3 | storedRef | (extendedId << 32);
	const bool success = atomicUpdate(hcAddress, REF_EMPTY, newRef);
	if (success) {
		++nContents;
	}

	return success;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) {
	assert (hcAddress < 1ul << DIM);
	assert (((reinterpret_cast<uintptr_t>(subnode) & 3) == 0) && "last 2 bits must not be set!");

	// 10 -> pointer to subnode
	const uintptr_t newRef =  2 | reinterpret_cast<uintptr_t>(subnode);
	const bool success = atomicUpdate(hcAddress, REF_EMPTY, newRef);
	if (success) {
		++nContents;
	}

	return success;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::linearCopyFromOther(unsigned long hcAddress, uintptr_t pointer) {
	insertAtAddress(hcAddress, pointer);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::linearCopyFromOther(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) {
	insertAtAddress(hcAddress, suffixStartBlockIndex, id);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::linearCopyFromOther(unsigned long hcAddress, unsigned long suffix, int id) {
	insertAtAddress(hcAddress, suffix, id);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::linearCopyFromOther(unsigned long hcAddress, const Node<DIM>* const subnode) {
	insertAtAddress(hcAddress, subnode);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::updateAddress(const Node<DIM>* const subnode, const NodeAddressContent<DIM>& prevContent) {
	assert (((reinterpret_cast<uintptr_t>(subnode) & 3) == 0) && "last 2 bits must not be set!");

	uintptr_t oldRef = reconstructReference(prevContent);
	// 10 -> pointer to subnode
	const uintptr_t newRef =  2 | reinterpret_cast<uintptr_t>(subnode);
	return atomicUpdate(prevContent.address, oldRef, newRef);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, uintptr_t pointer) {
	assert (hcAddress < 1ul << DIM);
	assert ((pointer & 3) == 0 && "last 2 bits must not be set!");
	assert (pointer != 0 && "cannot store null pointers");

	uintptr_t oldRef = REF_EMPTY;
	const bool success = atomic_compare_exchange_strong(&references_[hcAddress], &oldRef, pointer);
	if (success) {
		++nContents;
	}

	return success;
}


template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::updateAddress(uintptr_t pointer, const NodeAddressContent<DIM>& prevContent) {
	assert ((pointer & 3) == 0 && "last 2 bits must not be set!");
	assert (pointer != 0 && "cannot store null pointers");
	uintptr_t oldRef = reconstructReference(prevContent);
	return atomicUpdate(prevContent.address, oldRef, pointer);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
inline bool AHC<DIM, PREF_BLOCKS>::atomicUpdate(unsigned long hcAddress, uintptr_t oldRef, uintptr_t newRef) {
	return atomic_compare_exchange_strong(&references_[hcAddress], &oldRef, newRef);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
inline uintptr_t AHC<DIM, PREF_BLOCKS>::reconstructReference(const NodeAddressContent<DIM>& prevContent) const {
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

template <unsigned int DIM, unsigned int PREF_BLOCKS>
bool AHC<DIM, PREF_BLOCKS>::updateAddressToSpinlock(const NodeAddressContent<DIM>& prevContent) {
	uintptr_t oldPointer = reconstructReference(prevContent);
	return atomicUpdate(prevContent.address, oldPointer, REF_SPINLOCK);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::updateAddressFromSpinlock(unsigned long hcAddress, const Node<DIM>* const subnode) {
	const uintptr_t pointer = 2 | reinterpret_cast<uintptr_t>(subnode);
	const bool success = atomicUpdate(hcAddress, REF_SPINLOCK, pointer);
	assert (success);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::updateAddressFromSpinlock(unsigned long hcAddress, uintptr_t pointer) {
	const bool success = atomicUpdate(hcAddress, REF_SPINLOCK, pointer);
	assert (success);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::begin() const {
	AHCIterator<DIM, PREF_BLOCKS>* it = new AHCIterator<DIM, PREF_BLOCKS>(*this);
	it->setToBegin();
	return it;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::it(unsigned long hcAddress) const {
	return new AHCIterator<DIM, PREF_BLOCKS>(hcAddress, *this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::end() const {
	NodeIterator<DIM>* it = new AHCIterator<DIM, PREF_BLOCKS>(*this);
	it->setToEnd();
	return it;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::accept(Visitor<DIM>* visitor, size_t depth, unsigned int index)  {
	visitor->visit(this, depth, index);
	TNode<DIM, PREF_BLOCKS>::accept(visitor, depth, index);
}

#endif /* SRC_AHC_H_ */
