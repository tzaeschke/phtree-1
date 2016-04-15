/*
 * LHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <assert.h>
#include <utility>
#include "AHC.h"
#include "LHC.h"
#include "../iterators/LHCIterator.h"
#include "../visitors/Visitor.h"

using namespace std;

LHC::LHC(size_t dim, size_t valueLength) :
		Node(dim, valueLength) {
	prefix_ = vector<bool>();
	longestSuffix_ = 0;
}

LHC::~LHC() {
	for (auto entry : sortedContents_) {
		entry.second.suffix.clear();
	}

	sortedContents_.clear();
}

void LHC::recursiveDelete() {
	for (auto entry : sortedContents_) {
		if (entry.second.subnode) {
			entry.second.subnode->recursiveDelete();
		}
	}

	delete this;
}

NodeAddressContent LHC::lookup(unsigned long address) {
	assert (address < 1<<dim_);
	LHCAddressContent* contentRef = lookupReference(address);
	NodeAddressContent content;
	if (contentRef) {
		content.exists = true;
		content.address = address;
		content.hasSubnode = contentRef->hasSubnode;
		content.subnode = contentRef->subnode;
		content.suffix = NULL;
		if (!contentRef->hasSubnode) {
			content.suffix = &(contentRef->suffix);
			content.id = contentRef->id;
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
	assert (hcAddress < 1<<dim_);
	map<long, LHCAddressContent>::iterator it = sortedContents_.find(
			hcAddress);
	bool contained = it != sortedContents_.end();
	if (contained) {
		return &((*it).second);
	} else {
		return NULL;
	}
}

void LHC::insertAtAddress(unsigned long hcAddress, vector<bool>* suffix, int id) {
	assert (hcAddress < 1<<dim_);
	assert (suffix);

	LHCAddressContent* content = lookupReference(hcAddress);
	if (!content) {
		sortedContents_.emplace(piecewise_construct,
				forward_as_tuple(hcAddress),
				forward_as_tuple(suffix, id));
	} else {
		assert (!content->hasSubnode && "cannot insert a suffix at a position with a subnode");

		content->hasSubnode = false;
		content->suffix = *suffix;
		content->subnode = NULL;
	}

	if (suffix->size() / dim_ > longestSuffix_) {
		longestSuffix_ = suffix->size() / dim_;
	}

	assert (lookup(hcAddress).address == hcAddress);
	assert (lookup(hcAddress).suffix->size() % dim_ == 0);
}

void LHC::insertAtAddress(unsigned long hcAddress, Node* subnode) {
	assert (hcAddress < 1<<dim_);
	assert (subnode);

	LHCAddressContent* content = lookupReference(hcAddress);
		if (!content) {
			sortedContents_.emplace(hcAddress, subnode);
		} else {
			if (!content->hasSubnode && content->suffix.size() == longestSuffix_) {
				// before insertion this was the longest suffix
				// TODO efficiently find longest remaining suffix
				longestSuffix_ = 0;
				for (auto const content : sortedContents_) {
					if (!content.second.hasSubnode
							&& (content.second.suffix.size() / dim_) > longestSuffix_) {
						longestSuffix_ = content.second.suffix.size() / dim_;
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

void LHC::accept(Visitor* visitor, size_t depth) {
	visitor->visit(this, depth);
	Node::accept(visitor, depth);
}

ostream& LHC::output(ostream& os, size_t depth) {
	os << "LHC";
	Entry prefix(prefix_, dim_, 0);
	os << " | prefix: " << prefix << endl;

	for (auto const content : sortedContents_) {
		for (size_t i = 0; i < depth; i++) {os << "-";}
		os << " " << content.first << ": ";

		if (content.second.hasSubnode) {
			// print subnode
			content.second.subnode->output(os, depth + 1);
		} else {
			// print suffix
			Entry suffix(content.second.suffix, dim_, 0);
			os << " suffix: " << suffix;
			os << " (id: " << content.second.id << ")" << endl;
		}
	}
	return os;
}
