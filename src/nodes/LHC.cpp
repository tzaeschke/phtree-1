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
		if (entry.second->hasSubnode) {
			delete entry.second->subnode;
		} else if (entry.second->contained) {
			delete entry.second->suffix;
		}
		delete entry.second;
	}
	sortedContents_->clear();
}

NodeAddressContent* LHC::lookup(long address) {
	map<long, NodeAddressContent*>::iterator it = sortedContents_->find(
			address);
	bool contained = it != sortedContents_->end();
	NodeAddressContent *content;
	if (contained) {
		content = it->second;
	} else {
		content = new NodeAddressContent();
		content->contained = false;
	}

	assert ((!content->contained || (content->hasSubnode || content->suffix->size() == dim_))
						&& "the suffix dimensionality should always be the same as the node's");
	return content;
}

void LHC::insertAtAddress(long hcAddress, vector<vector<bool>>* suffix) {
	NodeAddressContent* content = lookup(hcAddress);
	if (!content->contained) {
		NodeAddressContent* newContent = new NodeAddressContent();
		newContent->address = hcAddress;
		newContent->contained = true;
		newContent->hasSubnode = false;
		newContent->suffix = suffix;
		sortedContents_->insert(
				pair<long, NodeAddressContent*>(hcAddress, newContent));
	} else {
		content->hasSubnode = false;
		content->suffix = suffix;
	}

	if (suffix->at(0).size() > longestSuffix_) {
		longestSuffix_ = suffix->at(0).size();
	}

	assert (lookup(hcAddress)->suffix->size() == dim_);
}

void LHC::insertAtAddress(long hcAddress, Node* subnode) {
	NodeAddressContent* content = lookup(hcAddress);
		if (!content->contained) {
			NodeAddressContent* newContent = new NodeAddressContent();
			newContent->address = hcAddress;
			newContent->contained = true;
			newContent->hasSubnode = true;
			newContent->subnode = subnode;
			sortedContents_->insert(
					pair<long, NodeAddressContent*>(hcAddress, newContent));
		} else {
			if (!content->hasSubnode && content->suffix->size() == longestSuffix_) {
				// before insertion this was the longest suffix
				// TODO efficiently find longest remaining suffix
				longestSuffix_ = 0;
				for (auto const &content : (*sortedContents_)) {
					if (content.second->contained
							&& !content.second->hasSubnode
							&& content.second->suffix->at(0).size() > longestSuffix_) {
						longestSuffix_ = content.second->suffix->at(0).size();
					}
				}
			}

			content->hasSubnode = true;
			content->subnode = subnode;
//TODO remove previous suffix memory			content->suffix->clear();
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
//TODO	delete this;
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
