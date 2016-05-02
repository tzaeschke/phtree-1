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
#include "nodes/Node.h"
#include "nodes/LHCAddressContent.h"
#include "util/MultiDimBitset.h"

template <unsigned int DIM>
class LHCIterator;

template <unsigned int DIM>
class AssertionVisitor;

template <unsigned int DIM>
class LHC: public Node<DIM> {
	friend class LHCIterator<DIM>;
	friend class AssertionVisitor<DIM>;
	friend class SizeVisitor<DIM>;
public:
	LHC(size_t valueLength);
	virtual ~LHC();
	NodeIterator<DIM>* begin() override;
	NodeIterator<DIM>* end() override;
	std::ostream& output(std::ostream& os, size_t depth) override;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) override;
	virtual void recursiveDelete() override;
	virtual size_t getNumberOfContents() const override;

protected:
	std::map<unsigned long, LHCAddressContent<DIM>> sortedContents_;
	size_t longestSuffix_;

	NodeAddressContent<DIM> lookup(unsigned long address) override;
	MultiDimBitset<DIM>* insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) override;
	void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) override;
	Node<DIM>* adjustSize() override;

private:
	LHCAddressContent<DIM>* lookupReference(unsigned long hcAddress);
};

#include <assert.h>
#include <utility>
#include "nodes/AHC.h"
#include "nodes/LHC.h"
#include "iterators/LHCIterator.h"
#include "visitors/Visitor.h"

using namespace std;

template <unsigned int DIM>
LHC<DIM>::LHC(size_t valueLength) :
		Node<DIM>(valueLength), longestSuffix_(0) {
}

template <unsigned int DIM>
LHC<DIM>::~LHC() {
	for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
		auto entry = (*it).second;
		entry.suffix.clear();
	}

	sortedContents_.clear();
}

template <unsigned int DIM>
void LHC<DIM>::recursiveDelete() {
	for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
		auto entry = (*it).second;
		if (entry.subnode) {
			entry.subnode->recursiveDelete();
		}
	}

	delete this;
}

template <unsigned int DIM>
NodeAddressContent<DIM> LHC<DIM>::lookup(unsigned long address) {
	assert (address < 1uL << DIM);
	LHCAddressContent<DIM>* contentRef = lookupReference(address);
	NodeAddressContent<DIM> content;
	if (contentRef) {
		content.exists = true;
		content.address = address;
		content.hasSubnode = contentRef->hasSubnode;
		if (!contentRef->hasSubnode) {
			content.suffix = &(contentRef->suffix);
			content.id = contentRef->id;
		} else {
			content.subnode = contentRef->subnode;
		}
	} else {
		content.exists = false;
		content.hasSubnode = false;
	}

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() % DIM == 0))
						&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

template <unsigned int DIM>
LHCAddressContent<DIM>* LHC<DIM>::lookupReference(unsigned long hcAddress) {
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

template <unsigned int DIM>
MultiDimBitset<DIM>* LHC<DIM>::insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) {
	assert (hcAddress < 1uL << DIM);

	LHCAddressContent<DIM>* content = lookupReference(hcAddress);
	if (!content) {
		auto result = sortedContents_.emplace(hcAddress, id);
		assert (result.second);
		content = &((*(result.first)).second);
	} else {
		assert (!content->hasSubnode && "cannot insert a suffix at a position with a subnode");

		content->hasSubnode = false;
		content->subnode = NULL;
	}

	if (suffixLength > longestSuffix_) {
		longestSuffix_ = suffixLength;
	}

	assert (lookup(hcAddress).address == hcAddress);
	return &(content->suffix);
}

template <unsigned int DIM>
void LHC<DIM>::insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) {
	assert (hcAddress < 1uL << DIM);
	assert (subnode);

	LHCAddressContent<DIM>* content = lookupReference(hcAddress);
		if (!content) {
			sortedContents_.emplace(hcAddress, subnode);
		} else {
			if (!content->hasSubnode && content->suffix.size() == longestSuffix_) {
				// before insertion this was the longest suffix
				// TODO efficiently find longest remaining suffix
				longestSuffix_ = 0;

				for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
						auto content = (*it).second;
						if (!content.hasSubnode
								&& (content.suffix.size() / DIM) > longestSuffix_) {
							longestSuffix_ = content.suffix.size() / DIM;
						}
				}
			}

			content->hasSubnode = true;
			content->subnode = subnode;
			content->suffix.clear();
		}

		assert (lookup(hcAddress).address == hcAddress);
}

template <unsigned int DIM>
Node<DIM>* LHC<DIM>::adjustSize() {
	// TODO find more precise threshold depending on AHC and LHC representation!
	const size_t n = sortedContents_.size();
	const size_t k = DIM;
	const size_t ls = longestSuffix_;
	const double conversionThreshold = (1uL << k) * ls / (k + ls);
	if (n < conversionThreshold) {
		return this;
	} else {
		AHC<DIM>* convertedNode = new AHC<DIM>(*this);
		return convertedNode;
	}
}

template <unsigned int DIM>
NodeIterator<DIM>* LHC<DIM>::begin() {
	return new LHCIterator<DIM>(*this);
}

template <unsigned int DIM>
NodeIterator<DIM>* LHC<DIM>::end() {
	return new LHCIterator<DIM>(1uL << DIM, *this);
}

template <unsigned int DIM>
size_t LHC<DIM>::getNumberOfContents() const {
	return sortedContents_.size();
}

template <unsigned int DIM>
void LHC<DIM>::accept(Visitor<DIM>* visitor, size_t depth) {
	visitor->visit(this, depth);
	Node<DIM>::accept(visitor, depth);
}

template <unsigned int DIM>
ostream& LHC<DIM>::output(ostream& os, size_t depth) {
	os << "LHC";
	Entry<DIM> prefix(this->prefix_, DIM, 0);
	os << " | prefix: " << prefix << endl;


	for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
		auto content = (*it).second;
		size_t address = (*it).first;
		for (size_t i = 0; i < depth; i++) {os << "-";}
		os << " " << address << ": ";

		if (content.hasSubnode) {
			// print subnode
			content.subnode->output(os, depth + 1);
		} else {
			// print suffix
			Entry<DIM> suffix(content.suffix, DIM, 0);
			os << " suffix: " << suffix;
			os << " (id: " << content.id << ")" << endl;
		}
	}
	return os;
}

#endif /* LHC_H_ */
