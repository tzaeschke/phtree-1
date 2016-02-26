/*
 * LHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "LHC.h"
#include <utility>

LHC::LHC(size_t dim, size_t valueLength) :
		Node(dim, valueLength) {
	sortedContents_ = new map<long, Node::NodeAddressContent*>();
	prefix_ = vector<vector<bool>>(dim_);
}

LHC::~LHC() {
	for (auto const &entry : (*sortedContents_)) {
		delete entry.second;
	}
	delete sortedContents_;
	delete &prefix_;
}

Node::NodeAddressContent LHC::lookup(long address) {
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
			// TODO persisted?
			content.hasSubnode = true;
			content.subnode = subnode;
		}
}

ostream& LHC::output(ostream& os, size_t depth) {
	os << "LHC" << endl;
	for (auto const &content : (*sortedContents_)) {
		if (content.second->hasSubnode) {
			for (int i = 0; i < depth; i++) { os << "-"; }
			content.second->subnode->output(os, depth + 1);
		}
	}
	return os;
}
