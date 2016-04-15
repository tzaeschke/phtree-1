/*
 * DynamicNodeOperationsUtil.cpp
 *
 *  Created on: Apr 7, 2016
 *      Author: max
 */

#include <assert.h>
#include <stdexcept>
#include "util/DynamicNodeOperationsUtil.h"
#include "util/SpatialSelectionOperationsUtil.h"
#include "util/MultiDimBitTool.h"
#include "util/NodeTypeUtil.h"
#include "nodes/Node.h"
#include "nodes/NodeAddressContent.h"
#include "boost/dynamic_bitset.hpp"

using namespace std;

#define DEBUG false

void DynamicNodeOperationsUtil::createSubnodeWithExistingSuffix(size_t dim,
		size_t bitLength, size_t currentIndex, Node* currentNode,
		NodeAddressContent content, const Entry* entry) {
	Node* subnode = NodeTypeUtil::determineNodeType(dim, bitLength, 2);

	// set longest common prefix in subnode
	int prefixLength = MultiDimBitTool::setLongestCommonPrefix(
			&subnode->prefix_, dim, currentIndex + 1, &entry->values_,
			content.suffix);

	// address in subnode starts after common prefix
	long insertEntryHCAddress = MultiDimBitTool::interleaveBits(
			currentIndex + 1 + prefixLength, entry);
	long existingEntryHCAddress = MultiDimBitTool::interleaveBits(prefixLength, dim,
			content.suffix);
	assert(insertEntryHCAddress != existingEntryHCAddress); // otherwise there would have been a longer prefix

	// add remaining bits after prefix and addresses as suffixes
	boost::dynamic_bitset<>* insertEntryPrefix = new boost::dynamic_bitset<>();
	boost::dynamic_bitset<>* exisitingEntryPrefix = new boost::dynamic_bitset<>();
	MultiDimBitTool::removeFirstBits(currentIndex + 1 + prefixLength + 1, dim,
			&(entry->values_), insertEntryPrefix);
	MultiDimBitTool::removeFirstBits(prefixLength + 1, dim, content.suffix,
			exisitingEntryPrefix);

	currentNode->insertAtAddress(content.address, subnode);
	subnode->insertAtAddress(insertEntryHCAddress, insertEntryPrefix,
			entry->id_);
	subnode->insertAtAddress(existingEntryHCAddress, exisitingEntryPrefix,
			content.id);

	// no need to adjust size because the correct node type was already provided
	assert (currentNode->lookup(content.address).subnode == subnode);
	assert (*subnode->lookup(existingEntryHCAddress).suffix == *exisitingEntryPrefix);
	assert (*subnode->lookup(insertEntryHCAddress).suffix == *insertEntryPrefix);

	// TODO the suffixes are stored locally so they were copied: better use a pointer to the correct memory location in the node
	insertEntryPrefix->clear();
	delete insertEntryPrefix;
	exisitingEntryPrefix->clear();
	delete exisitingEntryPrefix;
}

Node* DynamicNodeOperationsUtil::insertSuffix(size_t dim, size_t currentIndex,
		size_t hcAddress, Node* currentNode, const Entry* entry) {
	boost::dynamic_bitset<>* suffix = new boost::dynamic_bitset<>;
	MultiDimBitTool::removeFirstBits(currentIndex + 1, dim, &(entry->values_),
			suffix);
	currentNode->insertAtAddress(hcAddress, suffix, entry->id_);

	Node* adjustedNode = currentNode->adjustSize();
	assert(adjustedNode);
	assert(adjustedNode->lookup(hcAddress).exists);
	assert (*adjustedNode->lookup(hcAddress).suffix == *suffix);
	assert (adjustedNode->lookup(hcAddress).id == entry->id_);

	// TODO the suffix is stored locally so it was copied: better use a pointer to the correct memory location in the node
	suffix->clear();
	delete suffix;
	return adjustedNode;
}

void DynamicNodeOperationsUtil::splitSubnodePrefix(size_t dim, size_t bitLength,
		size_t currentIndex, size_t differentAtIndex, Node* currentNode,
		NodeAddressContent content, const Entry* entry) {
	Node* oldSubnode = content.subnode;
	Node* newSubnode = NodeTypeUtil::determineNodeType(dim, bitLength, 2);

	unsigned long newSubnodeEntryHCAddress = MultiDimBitTool::interleaveBits(
			currentIndex + 1 + differentAtIndex, entry);
	unsigned long newSubnodePrefixDiffHCAddress = MultiDimBitTool::interleaveBits(
			differentAtIndex, dim, &(oldSubnode->prefix_));
	assert (newSubnodeEntryHCAddress != newSubnodePrefixDiffHCAddress);

	// move A part of old prefix to new subnode and remove [A | d] from old prefix
	MultiDimBitTool::duplicateFirstBits(differentAtIndex, dim,
			&(oldSubnode->prefix_), &(newSubnode->prefix_));
	MultiDimBitTool::removeFirstBits(differentAtIndex + 1, dim,
			&(oldSubnode->prefix_));

	boost::dynamic_bitset<>* newSubnodeEntryPrefix = new boost::dynamic_bitset<>();
	MultiDimBitTool::removeFirstBits(currentIndex + 1 + differentAtIndex + 1, dim,
			&(entry->values_), newSubnodeEntryPrefix);

	currentNode->insertAtAddress(content.address, newSubnode);
	newSubnode->insertAtAddress(newSubnodeEntryHCAddress, newSubnodeEntryPrefix,
			entry->id_);
	newSubnode->insertAtAddress(newSubnodePrefixDiffHCAddress, oldSubnode);

	// no need to adjust size because the old node remains and the new one already
	// has the correct size
	assert (currentNode->lookup(content.address).hasSubnode);
	assert (currentNode->lookup(content.address).subnode == newSubnode);
	assert (!newSubnode->lookup(newSubnodeEntryHCAddress).hasSubnode);
	assert (*newSubnode->lookup(newSubnodeEntryHCAddress).suffix == *newSubnodeEntryPrefix);
	assert (newSubnode->lookup(newSubnodePrefixDiffHCAddress).hasSubnode);
	assert (newSubnode->lookup(newSubnodePrefixDiffHCAddress).subnode == oldSubnode);
	// TODO the suffixes are stored locally so they were copied: better use a pointer to the correct memory location in the node
	newSubnodeEntryPrefix->clear();
	delete newSubnodeEntryPrefix;
}

Node* DynamicNodeOperationsUtil::insert(const Entry* entry, Node* rootNode,
		size_t dim, size_t bitLength) {

	size_t depth = 0;
	size_t lastHcAddress = 0;
	size_t index = 0;
	Node* lastNode = NULL;
	Node* currentNode = rootNode;
	Node* initialNode = rootNode;

	while (index < bitLength) {
		if (DEBUG)
			cout << "(depth " << depth << "): ";

		const size_t currentIndex = index + currentNode->getPrefixLength();
		const unsigned long hcAddress = MultiDimBitTool::interleaveBits(currentIndex, entry);
		const NodeAddressContent content = currentNode->lookup(hcAddress);
		assert(!content.exists || content.address == hcAddress);
		assert(
				(!content.exists || (content.subnode && !content.suffix)
						|| (!content.subnode && content.suffix))
						&& "before insertion there is either a subnode XOR a suffix at the address or the content does not exist");

		if (content.exists && content.hasSubnode) {
			assert(
					content.subnode && !content.suffix
							&& "should only have a subnode and no suffix");
			// node entry and subnode exist:
			// validate prefix of subnode
			// case 1 (entry contains prefix): recurse on subnode
			// case 2 (otherwise): split prefix at difference into two subnodes
			const size_t subnodePrefixLength = content.subnode->getPrefixLength();
			bool prefixIncluded = true;
			size_t differentBitAtPrefixIndex = 0;
			for (size_t i = 0; i < subnodePrefixLength * dim && prefixIncluded; ++i) {
				prefixIncluded = entry->values_[(currentIndex + 1) * dim + i]
									== content.subnode->prefix_[i];
				if (!prefixIncluded)
					differentBitAtPrefixIndex = i / dim;

			}

			if (prefixIncluded) {
				// recurse on subnode
				if (DEBUG)
					cout << "recurse -> ";
				lastHcAddress = hcAddress;
				lastNode = currentNode;
				currentNode = content.subnode;
				++depth;
				index = currentIndex + 1;
			} else {
				if (DEBUG)
					cout << "split subnode prefix" << endl;
				// split prefix of subnode [A | d | B] where d is the index of the first different bit
				// create new node with prefix A and only leave prefix B in old subnode
				splitSubnodePrefix(dim, bitLength, currentIndex,
						differentBitAtPrefixIndex, currentNode, content, entry);

				break;
			}

		} else if (content.exists && !content.hasSubnode) {
			assert(
					!content.subnode && content.suffix
							&& "should only have a suffix and no subnode");
			if (DEBUG)
				cout << "create subnode with existing suffix" << endl;
			// node entry and suffix exist:
			// convert suffix to new node with prefix (longest common) + insert
			createSubnodeWithExistingSuffix(dim, bitLength, currentIndex,
					currentNode, content, entry);

			break;
		} else {
			if (DEBUG)
				cout << "insert" << endl;
			// node entry does not exist:
			// insert entry + suffix
			Node* adjustedNode = insertSuffix(dim, currentIndex, hcAddress,
					currentNode, entry);
			assert(adjustedNode);
			if (adjustedNode != currentNode && lastNode) {
				// the subnode changed: store the new one and delete the old
				lastNode->insertAtAddress(lastHcAddress, adjustedNode);
				delete currentNode;
				currentNode = adjustedNode;
			} else if (adjustedNode != currentNode) {
				// the root node changed
				initialNode = adjustedNode;
				currentNode = adjustedNode;
			}

			break;
		}
	}

	// validation only: lookup again after insertion
	size_t hcAddress = MultiDimBitTool::interleaveBits(index + currentNode->getPrefixLength(), entry);
	NodeAddressContent content = currentNode->lookup(hcAddress);
	assert(
			content.exists && content.address == hcAddress
					&& "after insertion the entry is always contained at the address");
	assert(
			((content.subnode && !content.suffix)
					|| (!content.subnode && content.suffix))
					&& "after insertion there is either a subnode XOR a suffix at the address");
	assert(
			(content.hasSubnode
					|| (index + currentNode->getPrefixLength() + 1
							+ currentNode->getSuffixSize(
									currentNode->lookup(hcAddress))
							== currentNode->valueLength_))
					&& "if there is a suffix for the entry the index + the current bit + suffix + prefix equals total bit width");
	assert (SpatialSelectionOperationsUtil::lookup(entry, initialNode, NULL).second == entry->id_);

	// the root node might have changed
	return initialNode;
}


