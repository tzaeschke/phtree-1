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
#include "nodes/TNode.h"
#include "nodes/LHCAddressContent.h"
#include "util/MultiDimBitset.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class LHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class LHC: public TNode<DIM, PREF_BLOCKS> {
	friend class LHCIterator<DIM, PREF_BLOCKS>;
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
	void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) override;
	void insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) override;
	void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) override;
	Node<DIM>* adjustSize() override;

protected:
	string getName() const override;

private:
	std::map<unsigned long, LHCAddressContent<DIM>> sortedContents_;

	LHCAddressContent<DIM>* lookupReference(unsigned long hcAddress);
};

#include <assert.h>
#include <utility>
#include "nodes/AHC.h"
#include "nodes/LHC.h"
#include "iterators/LHCIterator.h"
#include "visitors/Visitor.h"

using namespace std;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
LHC<DIM, PREF_BLOCKS>::LHC(size_t prefixLength) : TNode<DIM, PREF_BLOCKS>(prefixLength) {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
LHC<DIM, PREF_BLOCKS>::~LHC() {
	sortedContents_.clear();
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void LHC<DIM, PREF_BLOCKS>::recursiveDelete() {
	for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
		auto entry = (*it).second;
		if (entry.hasSubnode) {
			entry.subnode->recursiveDelete();
		}
	}

	delete this;
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
string LHC<DIM,PREF_BLOCKS>::getName() const {
	return "LHC";
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void LHC<DIM, PREF_BLOCKS>::lookup(unsigned long address, NodeAddressContent<DIM>& outContent) {
	assert (address < 1uL << DIM);
	LHCAddressContent<DIM>* contentRef = lookupReference(address);
	if (contentRef) {
		outContent.exists = true;
		outContent.address = address;
		outContent.hasSubnode = contentRef->hasSubnode;
		if (!contentRef->hasSubnode) {
			outContent.suffixStartBlock = contentRef->suffixStartBlock;
			outContent.id = contentRef->id;
		} else {
			outContent.subnode = contentRef->subnode;
		}
	} else {
		outContent.exists = false;
		outContent.hasSubnode = false;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
LHCAddressContent<DIM>* LHC<DIM, PREF_BLOCKS>::lookupReference(unsigned long hcAddress) {
	assert (hcAddress < 1uL << DIM);
	typename map<unsigned long, LHCAddressContent<DIM>>::iterator it = sortedContents_.find(
			hcAddress);
	bool contained = it != sortedContents_.end();
	if (contained) {
		return &((*it).second);
	} else {
		return NULL;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void LHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) {
	assert (hcAddress < 1uL << DIM);

	LHCAddressContent<DIM>* content = lookupReference(hcAddress);
	if (!content) {
		auto result = sortedContents_.emplace(piecewise_construct,
			forward_as_tuple(hcAddress),
			forward_as_tuple(startSuffixBlock, id));
		assert (result.second);
//		content = &((*(result.first)).second);
	} else {
		assert (!content->hasSubnode && "cannot insert a suffix at a position with a subnode");

		content->hasSubnode = false;
		content->suffixStartBlock = startSuffixBlock;
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void LHC<DIM, PREF_BLOCKS>::insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) {
	assert (hcAddress < 1uL << DIM);
	assert (subnode);

	LHCAddressContent<DIM>* content = lookupReference(hcAddress);
	if (!content) {
		sortedContents_.emplace(hcAddress, subnode);
	} else {
		content->hasSubnode = true;
		content->subnode = subnode;
	}

	assert (((NodeAddressContent<DIM>)TNode<DIM, PREF_BLOCKS>::lookup(hcAddress)).address == hcAddress);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
Node<DIM>* LHC<DIM, PREF_BLOCKS>::adjustSize() {
	// TODO find more precise threshold depending on AHC and LHC representation!
	const size_t n = sortedContents_.size();
	const size_t k = DIM;
	const double conversionThreshold = double(1uL << k) / (k);
	if (n < conversionThreshold) {
		return this;
	} else {
		AHC<DIM, PREF_BLOCKS>* convertedNode = new AHC<DIM, PREF_BLOCKS>(this);
		return convertedNode;
	}
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS>::begin() {
	return new LHCIterator<DIM, PREF_BLOCKS>(*this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
NodeIterator<DIM>* LHC<DIM, PREF_BLOCKS>::end() {
	return new LHCIterator<DIM, PREF_BLOCKS>(1uL << DIM, *this);
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
size_t LHC<DIM, PREF_BLOCKS>::getNumberOfContents() const {
	return sortedContents_.size();
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
void LHC<DIM, PREF_BLOCKS>::accept(Visitor<DIM>* visitor, size_t depth) {
	visitor->visit(this, depth);
	TNode<DIM, PREF_BLOCKS>::accept(visitor, depth);
}

#endif /* LHC_H_ */
