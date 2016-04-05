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
	prefix_ = vector<vector<bool>>(dim_);
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
	LHCAddressContent* contentRef = lookupReference(address);
	NodeAddressContent content;
	if (contentRef) {
		content.exists = true;
		content.address = address;
		content.hasSubnode = contentRef->hasSubnode;
		content.subnode = contentRef->subnode;
		content.suffix = NULL;
		if (!contentRef->suffix.empty())
			content.suffix = &(contentRef->suffix);
	} else {
		content.exists = false;
		content.hasSubnode = false;
	}

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() == dim_))
						&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

LHCAddressContent* LHC::lookupReference(unsigned long hcAddress) {
	map<long, LHCAddressContent>::iterator it = sortedContents_.find(
			hcAddress);
	bool contained = it != sortedContents_.end();
	if (contained) {
		return &((*it).second);
	} else {
		return NULL;
	}
}

void LHC::insertAtAddress(unsigned long hcAddress, vector<vector<bool>>* suffix) {
	LHCAddressContent* content = lookupReference(hcAddress);
	if (!content) {
		sortedContents_.emplace(hcAddress, suffix);
	} else {
		assert (!content->hasSubnode && "cannot insert a suffix at a position with a subnode");

		content->hasSubnode = false;
		content->suffix = *suffix;
		content->subnode = NULL;
	}

	if (suffix->at(0).size() > longestSuffix_) {
		longestSuffix_ = suffix->at(0).size();
	}

	assert (lookup(hcAddress).address == hcAddress);
	assert (lookup(hcAddress).suffix->size() == dim_);
}

void LHC::insertAtAddress(unsigned long hcAddress, Node* subnode) {
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
							&& content.second.suffix.at(0).size() > longestSuffix_) {
						longestSuffix_ = content.second.suffix.at(0).size();
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
	Entry prefix(prefix_);
	os << " | prefix: " << prefix << endl;

	for (auto const content : sortedContents_) {
		for (size_t i = 0; i < depth; i++) {os << "-";}
		os << " " << content.first << ": ";

		if (content.second.hasSubnode) {
			// print subnode
			content.second.subnode->output(os, depth + 1);
		} else {
			// print suffix
			Entry suffix(content.second.suffix);
			os << " suffix: " << suffix << endl;
		}
	}
	return os;
}
