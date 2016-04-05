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
	sortedContents_ = new map<long, NodeAddressContent*>();
	prefix_ = vector<vector<bool>>(dim_);
	longestSuffix_ = 0;
}

LHC::~LHC() {
	for (auto const &entry : (*sortedContents_)) {
		if (entry.second)
			delete entry.second;
	}
	sortedContents_->clear();
	delete sortedContents_;
}

void LHC::recursiveDelete() {
	for (auto const &entry : (*sortedContents_)) {
		if (entry.second->subnode) {
			entry.second->subnode->recursiveDelete();
		}
		if (entry.second->suffix) {
			entry.second->suffix->clear();
			delete entry.second->suffix;
		}
	}
	delete this;
}

NodeAddressContent LHC::lookup(long address) {
	NodeAddressContent* contentRef = lookupReference(address);
	NodeAddressContent content;
	if (contentRef) {
		content = *contentRef;
	} else {
		content.exists = false;
		content.hasSubnode = false;
	}

	assert ((!content.exists || (content.hasSubnode || content.suffix->size() == dim_))
						&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

NodeAddressContent* LHC::lookupReference(long hcAddress) {
	map<long, NodeAddressContent*>::iterator it = sortedContents_->find(
			hcAddress);
	bool contained = it != sortedContents_->end();
	if (contained) {
		return it->second;
	} else {
		return NULL;
	}
}

inline void clearPresentContentData(NodeAddressContent* content) {
	assert ((content->subnode && !content->suffix) || (!content->subnode && content->suffix));

	if (content->hasSubnode && content->subnode) {
		delete content->subnode;
		content->subnode = NULL;
	} else if (!content->hasSubnode && content->suffix) {
		content->suffix->clear();
		delete content->suffix;
		content->suffix = NULL;
	}
	assert (!content->subnode && !content->suffix);
}

void LHC::insertAtAddress(long hcAddress, vector<vector<bool>>* suffix) {
	NodeAddressContent* content = lookupReference(hcAddress);
	if (!content) {
		NodeAddressContent* newContent = new NodeAddressContent();
		newContent->exists = true;
		newContent->address = hcAddress;
		newContent->hasSubnode = false;
		newContent->suffix = suffix;
		sortedContents_->insert(
				pair<long, NodeAddressContent*>(hcAddress, newContent));
	} else {
		assert (content->exists);
		assert (!content->hasSubnode && "cannot insert a suffix at a position with a subnode");

		// clear previously stored data if present
		clearPresentContentData(content);

		content->hasSubnode = false;
		content->suffix = suffix;
	}

	if (suffix->at(0).size() > longestSuffix_) {
		longestSuffix_ = suffix->at(0).size();
	}

	assert (lookup(hcAddress).suffix->size() == dim_);
}

void LHC::insertAtAddress(long hcAddress, Node* subnode) {
	NodeAddressContent* content = lookupReference(hcAddress);
		if (!content) {
			NodeAddressContent* newContent = new NodeAddressContent();
			newContent->exists = true;
			newContent->address = hcAddress;
			newContent->hasSubnode = true;
			newContent->subnode = subnode;
			sortedContents_->insert(
					pair<long, NodeAddressContent*>(hcAddress, newContent));
		} else {
			assert (content->exists);

			if (!content->hasSubnode && content->suffix->size() == longestSuffix_) {
				// before insertion this was the longest suffix
				// TODO efficiently find longest remaining suffix
				longestSuffix_ = 0;
				for (auto const &content : (*sortedContents_)) {
					if (content.second
							&& !content.second->hasSubnode
							&& content.second->suffix->at(0).size() > longestSuffix_) {
						longestSuffix_ = content.second->suffix->at(0).size();
					}
				}
			}

			content->hasSubnode = true;
			content->subnode = subnode;
			content->suffix = NULL;
		}

}

Node* LHC::adjustSize() {
	// TODO find more precise threshold depending on AHC and LHC representation!
	size_t n = sortedContents_->size();
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

	for (auto const &content : (*sortedContents_)) {
		for (size_t i = 0; i < depth; i++) {os << "-";}
		os << " " << content.first << ": ";

		if (content.second->hasSubnode) {
			// print subnode
			content.second->subnode->output(os, depth + 1);
		} else {
			// print suffix
			Entry suffix(*(content.second->suffix));
			os << " suffix: " << suffix << endl;
		}
	}
	return os;
}
