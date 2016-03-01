/*
 * LHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <utility>
#include "AHC.h"
#include "LHC.h"
#include "LHCIterator.h"

LHC::LHC(size_t dim, size_t valueLength) :
		Node(dim, valueLength) {
	sortedContents_ = new map<long, NodeAddressContent*>();
	prefix_ = vector<vector<bool>>(dim_);
	longestSuffix_ = 0;
	highestAddress = -1;
}

LHC::~LHC() {
	for (auto const &entry : (*sortedContents_)) {
		delete entry.second;
	}
	delete sortedContents_;
	delete &prefix_;
}

NodeAddressContent LHC::lookup(long address) {
	map<long, NodeAddressContent*>::iterator it = sortedContents_->find(
			address);
	bool contained = it != sortedContents_->end();
	if (contained) {
		return *(it->second);
	} else {
		NodeAddressContent content;
		content.contained = false;
		return content;
	}
}

void LHC::insertAtAddress(long hcAddress, vector<vector<bool>>* suffix) {
	NodeAddressContent content = lookup(hcAddress);
	if (!content.contained) {
		NodeAddressContent* newContent = new NodeAddressContent();
		newContent->contained = true;
		newContent->hasSubnode = false;
		newContent->suffix = suffix;
		sortedContents_->insert(
				pair<long, NodeAddressContent*>(hcAddress, newContent));
	} else {
		// TODO persisted?
		content.hasSubnode = false;
		content.suffix = suffix;
	}

	if (suffix->at(0).size() > longestSuffix_) {
		longestSuffix_ = suffix->at(0).size();
	}
	if (hcAddress > highestAddress) {
		highestAddress = hcAddress;
	}
}

void LHC::insertAtAddress(long hcAddress, Node* subnode) {
	NodeAddressContent content = lookup(hcAddress);
		if (!content.contained) {
			NodeAddressContent* newContent = new NodeAddressContent();
			newContent->contained = true;
			newContent->hasSubnode = true;
			newContent->subnode = subnode;
			sortedContents_->insert(
					pair<long, NodeAddressContent*>(hcAddress, newContent));
		} else {
			if (!content.hasSubnode && content.suffix->size() == longestSuffix_) {
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

			// TODO persisted?
			content.hasSubnode = true;
			content.subnode = subnode;
			delete content.suffix;
		}

		if (hcAddress > highestAddress) {
			highestAddress = hcAddress;
		}
}

Node* LHC::adjustSize() {
	// TODO find exact threshold!
	size_t n = sortedContents_->size();
	size_t k = dim_;
	size_t ls = longestSuffix_;
	double conversionThreshold = (2<<k) * ls / (k + ls);
	if (n < conversionThreshold) {
		return this;
	} else {
		//TODO use alternative constructor
		AHC* convertedNode = new AHC(dim_, valueLength_);
		delete this;
		return convertedNode;
	}
}

NodeIterator LHC::begin() {
	return LHCIterator(*this);
}

NodeIterator LHC::end() {
	return LHCIterator(highestAddress, *this);
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
