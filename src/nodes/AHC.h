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
#include "nodes/TNode.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "nodes/AHCAddressContent.h"
#include "util/MultiDimBitset.h"

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
	AHC(size_t prefixLength);
	AHC(TNode<DIM, PREF_BLOCKS>* node);
	virtual ~AHC();
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
	static const size_t bitsPerBlock = sizeof(char) * 8;
	// stores 2 bits per entry: exists | hasSub
	char addresses_[1 + (2 * (1 << DIM) - 1) / bitsPerBlock];
	AHCAddressContent<DIM> contents_[1<<DIM];
	size_t nContents;

	void inline getExistsAndHasSub(unsigned long hcAddress, bool* exists, bool* hasSub) const;
	void inline setExists(unsigned long hcAddress, bool exists);
	void inline setHasSub(unsigned long hcAddress, bool hasSub);
};

#include <assert.h>
#include "nodes/AHC.h"
#include "iterators/AHCIterator.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "visitors/Visitor.h"

using namespace std;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::AHC(size_t prefixLength) : TNode<DIM, PREF_BLOCKS>(prefixLength), addresses_(), nContents(0) {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
AHC<DIM, PREF_BLOCKS>::AHC(TNode<DIM, PREF_BLOCKS>* other) : TNode<DIM, PREF_BLOCKS>(other), addresses_(), nContents(0) {

	// TODO use more efficient way to convert LHC->AHC
	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = other->end();
	for (it = other->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			contents_[content.address] = AHCAddressContent<DIM>(content.subnode);
		} else {
			assert (content.suffixStartBlock);
			contents_[content.address] = AHCAddressContent<DIM>(content.suffixStartBlock, content.id);
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
	bool hasSubnode = false;
	bool filled = false;
	for (size_t i = 0; i < 1uL << DIM; ++i) {
		getExistsAndHasSub(i, &filled, &hasSubnode);
		if (filled && hasSubnode) {
			assert (contents_[i].subnode);
			contents_[i].subnode->recursiveDelete();
		}
	}

	delete this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
string AHC<DIM,PREF_BLOCKS>::getName() const {
	return "AHC";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM,PREF_BLOCKS>::getExistsAndHasSub(unsigned long hcAddress, bool* exists, bool* hasSub) const {
	assert (hcAddress < (1 << DIM));

	if (exists) {
		const unsigned int existsBits = hcAddress * 2;
		const unsigned int existsBlockIndex = existsBits / bitsPerBlock;
		const unsigned int existsBitIndex = existsBits % bitsPerBlock;
		const char existsBlock = addresses_[existsBlockIndex];
		(*exists) = (existsBlock >> existsBitIndex) & 1;
	}

	if (hasSub) {
		const unsigned int hasSubBits = hcAddress * 2 + 1;
		const unsigned int hasSubBlockIndex = hasSubBits / bitsPerBlock;
		const unsigned int hasSubBitIndex = hasSubBits % bitsPerBlock;
		const char hasSubBlock = addresses_[hasSubBlockIndex];
		(*hasSub) = (hasSubBlock >> hasSubBitIndex) & 1;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM,PREF_BLOCKS>::setExists(unsigned long hcAddress, bool exists) {
	assert (hcAddress < (1 << DIM));

	const unsigned int existsBits = hcAddress * 2;
	const unsigned int existsBlockIndex = existsBits / bitsPerBlock;
	const unsigned int existsBitIndex = existsBits % bitsPerBlock;

	if (exists) {
		addresses_[existsBlockIndex] |= 1 << existsBitIndex;
	} else {
		addresses_[existsBlockIndex] &= ~(1 << existsBitIndex);
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM,PREF_BLOCKS>::setHasSub(unsigned long hcAddress, bool hasSub) {
	assert (hcAddress < (1 << DIM));

	const unsigned int hasSubBits = hcAddress * 2 + 1;
	const unsigned int hasSubBlockIndex = hasSubBits / bitsPerBlock;
	const unsigned int hasSubBitIndex = hasSubBits % bitsPerBlock;

	if (hasSub) {
		addresses_[hasSubBlockIndex] |= 1 << hasSubBitIndex;
	} else {
		addresses_[hasSubBlockIndex] &= ~(1 << hasSubBitIndex);
	}
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
void AHC<DIM, PREF_BLOCKS>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent) {
	assert (address < 1uL << DIM);

	outContent.address = address;
	getExistsAndHasSub(address, &outContent.exists, &outContent.hasSubnode);

	if (outContent.exists) {
		if (outContent.hasSubnode) {
			outContent.subnode = contents_[address].subnode;
		} else {
			outContent.suffixStartBlock = contents_[address].suffixStartBlock;
			outContent.id = contents_[address].id;
		}
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, unsigned long* suffixStartBlock, int id) {
	assert (hcAddress < 1ul << DIM);

	bool exists = false;
	bool hasSub = false;
	getExistsAndHasSub(hcAddress, &exists, &hasSub);

	if (!exists) {
		nContents++;
		setExists(hcAddress, true);
	}

	if (hasSub)
		setHasSub(hcAddress, false);

	contents_[hcAddress].id = id;
	contents_[hcAddress].suffixStartBlock = suffixStartBlock;
	assert(((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).id == id);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) {
	assert (hcAddress < 1ul << DIM);

	bool exists = false;
	bool hasSub = false;
	getExistsAndHasSub(hcAddress, &exists, &hasSub);

	if (!exists) {
		nContents++;
		setExists(hcAddress, true);
	}

	if (!hasSub) {
		setHasSub(hcAddress, true);
	}

	contents_[hcAddress].subnode = subnode;
	assert(((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).subnode == subnode);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
Node<DIM>* AHC<DIM, PREF_BLOCKS>::adjustSize() {
	// TODO currently there is no need to switch from AHC to LHC because there is no delete function
	return this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::begin() {
	return new AHCIterator<DIM, PREF_BLOCKS>(*this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* AHC<DIM, PREF_BLOCKS>::end() {
	return new AHCIterator<DIM, PREF_BLOCKS>(1uL << DIM, *this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void AHC<DIM, PREF_BLOCKS>::accept(Visitor<DIM>* visitor, size_t depth)  {
	visitor->visit(this, depth);
	TNode<DIM, PREF_BLOCKS>::accept(visitor, depth);
}

#endif /* SRC_AHC_H_ */
