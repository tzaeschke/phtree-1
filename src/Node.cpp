/*
 * Node.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include <assert.h>
#include <stdexcept>
#include "Node.h"
#include "LHC.h"

using namespace std;

#define DEBUG false

Node::Node(size_t dim, size_t valueLength) {
	dim_ = dim;
	valueLength_ = valueLength;
}

Node::Node(Node* other) {
	dim_ = other->dim_;
	valueLength_ = other->valueLength_;
	prefix_ = other->prefix_;
}

Node::~Node() {
	prefix_.clear();
}

Node* Node::insert(Entry* e, size_t depth, size_t index) {
		if (DEBUG)
			cout << "(depth " << depth << "): ";
		size_t currentIndex = index + getPrefixLength();
		long hcAddress = interleaveBits(currentIndex, e);
		NodeAddressContent* content = lookup(hcAddress);

		Node* adjustedNode = this;

		if (content->contained && content->hasSubnode) {
			// node entry and subnode exist:
			// validate prefix of subnode
			// case 1 (entry contains prefix): recurse on subnode
			// case 2 (otherwise): split prefix at difference into two subnodes
			size_t subnodePrefixLength = content->subnode->getPrefixLength();
			bool prefixIncluded = true;
			long differentBitAtPrefixIndex = -1;
			for (int i = 0; i < subnodePrefixLength && prefixIncluded; i++) {
				for (size_t value = 0; value < dim_ && prefixIncluded; value++) {
					prefixIncluded = e->values_[value][currentIndex + 1 + i]
							== content->subnode->prefix_[value][i];
					if (!prefixIncluded)
						differentBitAtPrefixIndex = i;
				}
			}

			if (prefixIncluded) {
				// recurse on subnode
				if (DEBUG)
					cout << "recurse -> ";
				Node* adjustedSubnode = content->subnode->insert(e, depth + 1, currentIndex + 1);
				// TODO more efficient if index is passed for LHC!
				insertAtAddress(hcAddress, adjustedSubnode);
			} else {
				if (DEBUG)
					cout << "split subnode prefix" << endl;
				// split prefix of subnode [A | d | B] where d is the index of the first different bit
				// create new node with prefix A and only leave prefix B in old subnode
				Node* oldSubnode = content->subnode;
				Node* newSubnode = determineNodeType(dim_, valueLength_, 2);

				long newSubnodeEntryHCAddress = interleaveBits(currentIndex + 1 + differentBitAtPrefixIndex, e);
				long newSubnodePrefixDiffHCAddress = interleaveBits(differentBitAtPrefixIndex, &(oldSubnode->prefix_));

				// move A part of old prefix to new subnode and remove [A | d] from old prefix
				duplicateFirstBits(differentBitAtPrefixIndex, &(oldSubnode->prefix_), &(newSubnode->prefix_));
				removeFirstBits(differentBitAtPrefixIndex + 1, &(oldSubnode->prefix_));

				vector<vector<bool>>* newSubnodeEntryPrefix = new vector<vector<bool>>(dim_);
				removeFirstBits(currentIndex + 1 + differentBitAtPrefixIndex + 1, &(e->values_), newSubnodeEntryPrefix);

				insertAtAddress(hcAddress, newSubnode);
				newSubnode->insertAtAddress(newSubnodeEntryHCAddress, newSubnodeEntryPrefix);
				newSubnode->insertAtAddress(newSubnodePrefixDiffHCAddress, oldSubnode);
				// no need to adjust size because the old node remains and the new one already
				// has the correct size
			}
		} else if (content->contained && !content->hasSubnode) {
			if (DEBUG)
				cout << "create subnode with existing suffix" << endl;
			// node entry and suffix exist:
			// convert suffix to new node with prefix (longest common) + insert
			Node* subnode = determineNodeType(dim_, valueLength_, 2);

			// set longest common prefix in subnode
			int prefixLength = setLongestCommonPrefix(subnode, currentIndex + 1, &e->values_, content->suffix);

			// address in subnode starts after common prefix
			long insertEntryHCAddress = interleaveBits(currentIndex + 1 + prefixLength, e);
			long existingEntryHCAddress = interleaveBits(prefixLength, content->suffix);
			assert (insertEntryHCAddress != existingEntryHCAddress); // otherwise there would have been a longer prefix

			// add remaining bits after prefix and addresses as suffixes
			vector<vector<bool>>* insertEntryPrefix = new vector<vector<bool>>(dim_);
			vector<vector<bool>>* exisitingEntryPrefix = new vector<vector<bool>>(dim_);
			removeFirstBits(currentIndex + 1 + prefixLength + 1, &(e->values_), insertEntryPrefix);
			removeFirstBits(prefixLength + 1, content->suffix, exisitingEntryPrefix);

			insertAtAddress(hcAddress, subnode);
			subnode->insertAtAddress(insertEntryHCAddress, insertEntryPrefix);
			subnode->insertAtAddress(existingEntryHCAddress, exisitingEntryPrefix);
			// no need to adjust size because the correct node type was already provided
		} else {
			if (DEBUG)
				cout << "insert" << endl;
			// node entry does not exist:
			// insert entry + suffix
			vector<vector<bool>>* suffix = new vector<vector<bool>>(dim_);
			removeFirstBits(currentIndex + 1, &(e->values_), suffix);
			insertAtAddress(hcAddress, suffix);
			assert (lookup(hcAddress)->contained);

			adjustedNode = adjustSize();
		}

		// after insertion the entry is always contained
		assert (this->lookup(hcAddress)->contained);
		// if there is a suffix for the entry the index + the current bit + suffix + prefix equals total bit width
		assert (this->lookup(hcAddress)->hasSubnode
				|| (index + this->getPrefixLength() + 1 + this->getSuffixSize(this->lookup(hcAddress)) == this->valueLength_));
		return adjustedNode;
}

size_t Node::setLongestCommonPrefix(Node* nodeToSetTo, size_t startIndexEntry1,
		vector<vector<bool>>* entry1, vector<vector<bool>>* entry2) {
	assert (entry1->size() == entry2->size()
			&& "both entries must have the same dimensions");
	assert (entry1->at(0).size() > startIndexEntry1
			&& "the first entry must include the given index");
	assert (nodeToSetTo->prefix_.size() == dim_
			&& "the prefix must already have the same dimensions as the node");
	assert (nodeToSetTo->getPrefixLength() == 0
			&& "the prefix must not have been set before");

	bool allDimSame = true;
	size_t prefixLength = 0;
	for (size_t i = startIndexEntry1; i < valueLength_ && allDimSame; i++) {
		for (size_t val = 0; val < dim_ && allDimSame; val++)
			allDimSame = (*entry1)[val][i] == (*entry2)[val][i - startIndexEntry1];

		if (allDimSame)
			prefixLength++;
		for (size_t val = 0; val < dim_ && allDimSame; val++)
			nodeToSetTo->prefix_[val].push_back((*entry1)[val][i]);
	}

	assert (nodeToSetTo->prefix_.size() == dim_
			&& "afterwards the prefix should have the same dimensionality as the node");
	assert (nodeToSetTo->getPrefixLength() == prefixLength
			&& "should have set the correct prefix length");
	return prefixLength;
}


Node* Node::determineNodeType(size_t dim, size_t valueLength, size_t nDirectInserts) {
	// TODO determine node type dynamically depending on dim and #inserts
	LHC* node = new LHC(dim, valueLength);
	return node;
}

bool Node::lookup(Entry* e, size_t depth, size_t index) {
	if (DEBUG)
		cout << "depth " << depth << " -> ";

		// validate prefix
		for (size_t bit = 0; bit < getPrefixLength(); bit++) {
			for (size_t value = 0; value < e->values_.size(); value++) {
				if (e->values_[value][index + bit] != prefix_[value][bit]) {
					if (DEBUG)
						cout << "prefix missmatch" << endl;
					return false;
				}
			}
		}

		// validate HC address
		int currentIndex = index + getPrefixLength();
		long hcAddress = interleaveBits(currentIndex, e);
		NodeAddressContent* content = lookup(hcAddress);

		if (!content->contained) {
			if (DEBUG)
				cout << "HC address missmatch" << endl;
			return false;
		}

		// validate suffix or recurse
		if (content->hasSubnode) {
			return content->subnode->lookup(e, depth + 1, currentIndex + 1);
		} else {
			for (size_t bit = 0; bit < getSuffixSize(content); bit++) {
				for (size_t value = 0; value < e->values_.size(); value++) {
					if (e->values_[value][currentIndex + 1 + bit] != (*content->suffix)[value][bit]) {
						if (DEBUG)
							cout << "suffix missmatch" << endl;
						return false;
					}
				}
			}

			if (DEBUG)
				cout << "found" << endl;
			return true;
		}
}

size_t Node::getSuffixSize(NodeAddressContent* content) {
	if (content->hasSubnode || content->suffix->empty()) {
		return 0;
	} else {
		return content->suffix->at(0).size();
	}
}

size_t Node::getPrefixLength() {
	if (prefix_.size() == 0)
		return 0;
	else
		return prefix_[0].size();
}

long Node::interleaveBits(size_t index, Entry* e) {
	return interleaveBits(index, &(e->values_));
}

long Node::interleaveBits(size_t index, vector<vector<bool>>* values) {
	long hcAddress = 0;
	size_t max = values->size() - 1;
	for (size_t value = 0; value < values->size(); value++) {
		hcAddress |= (*values)[value][index] << (max - value);
	}
	return hcAddress;
}

void Node::removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *valuesFrom, vector<vector<bool>>* valuesTo) {
	assert (valuesTo->size() == dim_);
	assert (valuesTo->at(0).empty());

	for (size_t valueIndex = 0; valueIndex < valuesFrom->size(); valueIndex++) {
		size_t newLength = (*valuesFrom)[valueIndex].size() - nBitsToRemove;
		vector<bool>* value = new vector<bool>(newLength);
		value->reserve(newLength);
		valuesTo->at(valueIndex) = *value;
		for (size_t bit = 0; bit < newLength; bit++) {
			bool copiedBit = valuesFrom->at(valueIndex).at(nBitsToRemove + bit);
			valuesTo->at(valueIndex).at(bit) = copiedBit;
			//value->at(bit) = copiedBit;
		}
	}

	assert (valuesTo->size() == dim_); // should retain the dimension
	assert (valuesTo->at(0).size() == valuesFrom->at(0).size() - nBitsToRemove);
}

void Node::removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *values) {
	for (size_t i = 0; i < values->size(); i++) {
			values->at(i).erase(values->at(i).begin(),
					values->at(i).begin() + nBitsToRemove);
	}

	assert (values->size() == dim_);
}

void Node::duplicateFirstBits(size_t nBitsToDuplicate, vector<vector<bool>>* from,
		vector<vector<bool>>* to) {
	for (size_t i = 0; i < from->size(); i++) {
		for (size_t j = 0; j < nBitsToDuplicate; j++) {
			to->at(i).push_back(from->at(i).at(j));
		}
	}

	assert (to->size() == dim_);
	assert (to->at(0).size() == nBitsToDuplicate);
}

void Node::accept(Visitor* visitor, size_t depth) {
	for (NodeIterator* it = this->begin(); (*it) <= *(this->end()); ++(*it)) {
		NodeAddressContent content = *(*it);
		if (content.hasSubnode) {
			content.subnode->accept(visitor, depth + 1);
		}
	}
}

ostream& Node::output(ostream& os, size_t depth) {
	return os << "subclass should overwrite this";
}

ostream& operator <<(ostream& os, Node &node) {
	return node.output(os, 1);
}
