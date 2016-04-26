/*
 * LHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <assert.h>
#include <utility>
#include "nodes/AHC.h"
#include "nodes/LHC.h"
#include "iterators/LHCIterator.h"
#include "visitors/Visitor.h"

using namespace std;

LHC::LHC(size_t dim, size_t valueLength) :
		Node(dim, valueLength) {
	longestSuffix_ = 0;
}

LHC::~LHC() {
	for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
		auto entry = (*it).second;
		entry.suffix.clear();
	}

	sortedContents_.clear();
}

void LHC::recursiveDelete() {
	for (auto it = sortedContents_.begin(); it != sortedContents_.end(); ++it) {
		auto entry = (*it).second;
		if (entry.subnode) {
			entry.subnode->recursiveDelete();
		}
	}

	delete this;
}

NodeAddressContent LHC::lookup(unsigned long address) {
	assert (address < 1uL << dim_);
	LHCAddressContent* contentRef = lookupReference(address);
	NodeAddressContent content;
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

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() % dim_ == 0))
						&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

LHCAddressContent* LHC::lookupReference(unsigned long hcAddress) {
	assert (hcAddress < 1uL << dim_);
	map<unsigned long, LHCAddressContent>::iterator it = sortedContents_.find(
			hcAddress);
	bool contained = it != sortedContents_.end();
	if (contained) {
		return &((*it).second);
	} else {
		return NULL;
	}
}

MultiDimBitset* LHC::insertAtAddress(unsigned long hcAddress, size_t suffixLength, int id) {
	assert (hcAddress < 1uL << dim_);

	LHCAddressContent* content = lookupReference(hcAddress);
	if (!content) {
		auto result = sortedContents_.emplace(piecewise_construct,
				forward_as_tuple(hcAddress),
				forward_as_tuple(dim_, id));
		assert (result.second);
		content = &((*(result.first)).second);
	} else {
		assert (!content->hasSubnode && "cannot insert a suffix at a position with a subnode");

		content->hasSubnode = false;
		content->suffix.setDim(dim_);
		content->subnode = NULL;
	}

	if (suffixLength > longestSuffix_) {
		longestSuffix_ = suffixLength;
	}

	assert (lookup(hcAddress).address == hcAddress);
	return &(content->suffix);
}

void LHC::insertAtAddress(unsigned long hcAddress, Node* subnode) {
	assert (hcAddress < 1uL << dim_);
	assert (subnode);

	LHCAddressContent* content = lookupReference(hcAddress);
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
								&& (content.suffix.size() / dim_) > longestSuffix_) {
							longestSuffix_ = content.suffix.size() / dim_;
						}
				}
			}

			content->hasSubnode = true;
			content->subnode = subnode;
			content->suffix.clear();
		}

		assert (lookup(hcAddress).address == hcAddress);
}

Node* LHC::adjustSize() {
	// TODO find more precise threshold depending on AHC and LHC representation!
	size_t n = sortedContents_.size();
	size_t k = dim_;
	size_t ls = longestSuffix_;
	double conversionThreshold = (1<<k) * ls / (k + ls);
	if (n < conversionThreshold) {
		return this;
	} else {
		AHC* convertedNode = new AHC(*this);
		return convertedNode;
	}
}

NodeIterator* LHC::begin() {
	return new LHCIterator(*this);
}

NodeIterator* LHC::end() {
	return new LHCIterator(1<<dim_, *this);
}

size_t LHC::getNumberOfContents() const {
	return sortedContents_.size();
}

void LHC::accept(Visitor* visitor, size_t depth) {
	visitor->visit(this, depth);
	Node::accept(visitor, depth);
}

ostream& LHC::output(ostream& os, size_t depth) {
	os << "LHC";
	Entry prefix(prefix_, dim_, 0);
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
			Entry suffix(content.suffix, dim_, 0);
			os << " suffix: " << suffix;
			os << " (id: " << content.id << ")" << endl;
		}
	}
	return os;
}
