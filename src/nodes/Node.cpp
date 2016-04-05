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
#include "../util/MultiDimBitTool.h"
#include "../iterators/RangeQueryIterator.h"

using namespace std;

#define DEBUG false

Node::Node(size_t dim, size_t valueLength) {
	dim_ = dim;
	valueLength_ = valueLength;
}

Node::Node(Node* other) : prefix_(other->prefix_) {
	dim_ = other->dim_;
	valueLength_ = other->valueLength_;
}

Node::~Node() {
	prefix_.clear();
}

Node* Node::insert(Entry* e, size_t depth, size_t index) {
		if (DEBUG)
			cout << "(depth " << depth << "): ";
		size_t currentIndex = index + getPrefixLength();
		long hcAddress = MultiDimBitTool::interleaveBits(currentIndex, e);
		NodeAddressContent content = lookup(hcAddress);
		assert ((!content.exists || (content.subnode && !content.suffix) || (!content.subnode && content.suffix))
				&& "before insertion there is either a subnode XOR a suffix at the address or the content does not exist");

		Node* adjustedNode = this;

		if (content.exists && content.hasSubnode) {
			assert (content.subnode && !content.suffix && "should only have a subnode and no suffix");
			// node entry and subnode exist:
			// validate prefix of subnode
			// case 1 (entry contains prefix): recurse on subnode
			// case 2 (otherwise): split prefix at difference into two subnodes
			size_t subnodePrefixLength = content.subnode->getPrefixLength();
			bool prefixIncluded = true;
			long differentBitAtPrefixIndex = -1;
			for (size_t i = 0; i < subnodePrefixLength && prefixIncluded; i++) {
				for (size_t value = 0; value < dim_ && prefixIncluded; value++) {
					prefixIncluded = e->values_[value][currentIndex + 1 + i]
							== content.subnode->prefix_[value][i];
					if (!prefixIncluded)
						differentBitAtPrefixIndex = i;
				}
			}

			if (prefixIncluded) {
				// recurse on subnode
				if (DEBUG)
					cout << "recurse -> ";
				Node* adjustedSubnode = content.subnode->insert(e, depth + 1, currentIndex + 1);
				if (adjustedSubnode != content.subnode) {
					// the subnode changed: store the new one and delete the old
					insertAtAddress(hcAddress, adjustedSubnode);
					delete content.subnode;
				}
			} else {
				if (DEBUG)
					cout << "split subnode prefix" << endl;
				// split prefix of subnode [A | d | B] where d is the index of the first different bit
				// create new node with prefix A and only leave prefix B in old subnode
				Node* oldSubnode = content.subnode;
				Node* newSubnode = determineNodeType(dim_, valueLength_, 2);

				long newSubnodeEntryHCAddress = MultiDimBitTool::interleaveBits(currentIndex + 1 + differentBitAtPrefixIndex, e);
				long newSubnodePrefixDiffHCAddress = MultiDimBitTool::interleaveBits(differentBitAtPrefixIndex, &(oldSubnode->prefix_));

				// move A part of old prefix to new subnode and remove [A | d] from old prefix
				MultiDimBitTool::duplicateFirstBits(differentBitAtPrefixIndex, &(oldSubnode->prefix_), &(newSubnode->prefix_));
				MultiDimBitTool::removeFirstBits(differentBitAtPrefixIndex + 1, &(oldSubnode->prefix_));

				vector<vector<bool>>* newSubnodeEntryPrefix = new vector<vector<bool>>(dim_);
				MultiDimBitTool::removeFirstBits(currentIndex + 1 + differentBitAtPrefixIndex + 1, &(e->values_), newSubnodeEntryPrefix);

				insertAtAddress(hcAddress, newSubnode);
				newSubnode->insertAtAddress(newSubnodeEntryHCAddress, newSubnodeEntryPrefix);
				newSubnode->insertAtAddress(newSubnodePrefixDiffHCAddress, oldSubnode);

				// TODO the suffixes are stored locally so they were copied: better use a pointer to the correct memory location in the node
				newSubnodeEntryPrefix->clear();
				delete newSubnodeEntryPrefix;
				// no need to adjust size because the old node remains and the new one already
				// has the correct size
			}
		} else if (content.exists && !content.hasSubnode) {
			assert (!content.subnode && content.suffix && "should only have a suffix and no subnode");
			if (DEBUG)
				cout << "create subnode with existing suffix" << endl;
			// node entry and suffix exist:
			// convert suffix to new node with prefix (longest common) + insert
			Node* subnode = determineNodeType(dim_, valueLength_, 2);

			// set longest common prefix in subnode
			int prefixLength = MultiDimBitTool::setLongestCommonPrefix(&subnode->prefix_, currentIndex + 1, &e->values_, content.suffix);

			// address in subnode starts after common prefix
			long insertEntryHCAddress = MultiDimBitTool::interleaveBits(currentIndex + 1 + prefixLength, e);
			long existingEntryHCAddress = MultiDimBitTool::interleaveBits(prefixLength, content.suffix);
			assert (insertEntryHCAddress != existingEntryHCAddress); // otherwise there would have been a longer prefix

			// add remaining bits after prefix and addresses as suffixes
			vector<vector<bool>>* insertEntryPrefix = new vector<vector<bool>>(dim_);
			vector<vector<bool>>* exisitingEntryPrefix = new vector<vector<bool>>(dim_);
			MultiDimBitTool::removeFirstBits(currentIndex + 1 + prefixLength + 1, &(e->values_), insertEntryPrefix);
			MultiDimBitTool::removeFirstBits(prefixLength + 1, content.suffix, exisitingEntryPrefix);

			insertAtAddress(hcAddress, subnode);
			subnode->insertAtAddress(insertEntryHCAddress, insertEntryPrefix);
			subnode->insertAtAddress(existingEntryHCAddress, exisitingEntryPrefix);

			// TODO the suffixes are stored locally so they were copied: better use a pointer to the correct memory location in the node
			insertEntryPrefix->clear();
			delete insertEntryPrefix;
			exisitingEntryPrefix->clear();
			delete exisitingEntryPrefix;
			// no need to adjust size because the correct node type was already provided
		} else {
			if (DEBUG)
				cout << "insert" << endl;
			// node entry does not exist:
			// insert entry + suffix
			vector<vector<bool>>* suffix = new vector<vector<bool>>(dim_);
			MultiDimBitTool::removeFirstBits(currentIndex + 1, &(e->values_), suffix);
			insertAtAddress(hcAddress, suffix);
			assert (lookup(hcAddress).exists);

			// TODO the suffix is stored locally so it was copied: better use a pointer to the correct memory location in the node
			suffix->clear();
			delete suffix;

			adjustedNode = adjustSize();
			assert (adjustedNode);
		}

		// lookup again for validation
		content = this->lookup(hcAddress);
		assert (content.exists && content.address == hcAddress && "after insertion the entry is always contained at the address");
		assert (((content.subnode && !content.suffix) || (!content.subnode && content.suffix))
				&& "after insertion there is either a subnode XOR a suffix at the address");
		assert ((content.hasSubnode
				|| (index + this->getPrefixLength() + 1 + this->getSuffixSize(this->lookup(hcAddress)) == this->valueLength_))
				&& "if there is a suffix for the entry the index + the current bit + suffix + prefix equals total bit width");
		return adjustedNode;
}

Node* Node::determineNodeType(size_t dim, size_t valueLength, size_t nDirectInserts) {
	// TODO determine node type dynamically depending on dim and #inserts
	LHC* node = new LHC(dim, valueLength);
	return node;
}

bool Node::lookup(Entry* e, size_t depth, size_t index, vector<Node*>* visitedNodes) {
	if (DEBUG)
		cout << "depth " << depth << " -> ";

	if (visitedNodes != NULL)
		visitedNodes->push_back(this);

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
		long hcAddress = MultiDimBitTool::interleaveBits(currentIndex, e);
		NodeAddressContent content = lookup(hcAddress);

		if (!content.exists) {
			if (DEBUG)
				cout << "HC address missmatch" << endl;
			return false;
		}

		// validate suffix or recurse
		if (content.hasSubnode) {
			return content.subnode->lookup(e, depth + 1, currentIndex + 1, visitedNodes);
		} else {
			for (size_t bit = 0; bit < getSuffixSize(content); bit++) {
				for (size_t value = 0; value < e->values_.size(); value++) {
					if (e->values_[value][currentIndex + 1 + bit] != (*content.suffix)[value][bit]) {
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

RangeQueryIterator* Node::rangeQuery(Entry* lowerLeft, Entry* upperRight, size_t depth, size_t index) {
	vector<Node*>* visitedNodes = new vector<Node*>();
	this->lookup(lowerLeft, depth, index, visitedNodes);
	RangeQueryIterator* iterator = new RangeQueryIterator(visitedNodes, dim_, valueLength_, lowerLeft, upperRight);
	return iterator;
}

size_t Node::getSuffixSize(NodeAddressContent content) {
	if (content.hasSubnode || content.suffix->empty()) {
		return 0;
	} else {
		return content.suffix->at(0).size();
	}
}

size_t Node::getPrefixLength() {
	if (prefix_.size() == 0)
		return 0;
	else
		return prefix_[0].size();
}

void Node::accept(Visitor* visitor, size_t depth) {
	NodeIterator* it;
	NodeIterator* endIt = this->end();
	for (it = this->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent content = *(*it);
		assert (content.exists);
		if (content.hasSubnode) {
			content.subnode->accept(visitor, depth + 1);
		}
	}

	delete it;
	delete endIt;
}

ostream& Node::output(ostream& os, size_t depth) {
	return os << "subclass should overwrite this";
}

ostream& operator <<(ostream& os, Node &node) {
	return node.output(os, 1);
}
