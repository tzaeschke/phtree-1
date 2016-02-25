/*
 * AHC.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#include "AHC.h"

AHC::AHC(int dim, int valueLength) {
	valueLength_ = valueLength;
	dim_ = dim;
	long maxElements = 1 << dim;
	filled_ = vector<bool>(maxElements, false);
	hasSubnode_ = vector<bool>(maxElements, false);
	subnodes_ = vector<Node*>(maxElements);
	suffixes_ = vector<vector<vector<bool>>>(maxElements);
	prefix_ = vector<vector<bool>>(dim_);
}

AHC::~AHC() {
	delete &hasSubnode_;
	delete &filled_;
	for (size_t i = 0; i < subnodes_.size(); i++) {
		delete subnodes_[i];
	}
	delete &subnodes_;
	delete &suffixes_;
}

bool AHC::lookup(Entry* e, int depth, int index) {
	cout << "AHC depth " << depth << " -> ";

	// validate prefix
	for (size_t bit = 0; bit < getPrefixLength(); bit++) {
		for (size_t value = 0; value < e->values_.size(); value++) {
			if (e->values_[value][index + bit] != prefix_[value][bit]) {
				cout << "prefix missmatch" << endl;
				return false;
			}
		}
	}

	// validate HC address
	int currentIndex = index + getPrefixLength();
	long hcAddress = interleaveBits(currentIndex, e);
	if (!filled_[hcAddress]) {
		cout << "HC address missmatch" << endl;
		return false;
	}

	// validate suffix or recurse
	if (hasSubnode_[hcAddress]) {
		return subnodes_[hcAddress]->lookup(e, depth + 1, currentIndex + 1);
	} else {
		for (size_t bit = 0; bit < getSuffixSize(hcAddress); bit++) {
			for (size_t value = 0; value < e->values_.size(); value++) {
				if (e->values_[value][currentIndex + 1 + bit]
						!= suffixes_[hcAddress][value][bit]) {
					cout << "suffix missmatch" << endl;
					return false;
				}
			}
		}

		cout << "found" << endl;
		return true;
	}
}

void AHC::insert(Entry* e, int depth, int index) {
	cout << "AHC node (depth" << depth << ") ";
	int currentIndex = index + getPrefixLength();
	long hcAddress = interleaveBits(currentIndex, e);

	if (filled_[hcAddress] && hasSubnode_[hcAddress]) {
		// node entry and subnode exist:
		// validate prefix of subnode
		// case 1 (entry contains prefix): recurse on subnode
		// case 2 (otherwise): split prefix at difference into two subnodes
		int subnodePrefixLength = subnodes_[hcAddress]->getPrefixLength();
		bool prefixIncluded = true;
		int differentBitAtPrefixIndex = -1;
		for (int i = 0; i < subnodePrefixLength && prefixIncluded; i++) {
			for (int value = 0; value < dim_ && prefixIncluded; value++) {
				prefixIncluded = e->values_[value][currentIndex + 1 + i]
						== subnodes_[hcAddress]->prefix_[value][i];
				if (!prefixIncluded)
					differentBitAtPrefixIndex = i;
			}
		}

		if (prefixIncluded) {
			// recurse on subnode
			cout << "recurse" << endl;
			subnodes_[hcAddress]->insert(e, depth + 1, currentIndex + 1);
		} else {
			cout << "split subnode prefix" << endl;
			// split prefix of subnode [A | d | B] where d is the index of the first different bit
			// create new node with prefix A and only leave prefix B in old subnode
			Node* oldSubnode = subnodes_[hcAddress];
			AHC* newSubnode = new AHC(dim_, valueLength_);

			long newSubnodeEntryHCAddress = interleaveBits(
					currentIndex + 1 + differentBitAtPrefixIndex, e);
			long newSubnodePrefixDiffHCAddress = interleaveBits(
					differentBitAtPrefixIndex, &(oldSubnode->prefix_));

			newSubnode->filled_[newSubnodeEntryHCAddress] = true;
			newSubnode->filled_[newSubnodePrefixDiffHCAddress] = true;
			newSubnode->hasSubnode_[newSubnodePrefixDiffHCAddress] = true;
			subnodes_[hcAddress] = newSubnode;

			// move A part of old prefix to new subnode and remove [A | d] from old prefix
			duplicateFirstBits(differentBitAtPrefixIndex, oldSubnode->prefix_,
					&(newSubnode->prefix_));
			removeFirstBits(differentBitAtPrefixIndex + 1,
					&(oldSubnode->prefix_));

			removeFirstBits(currentIndex + 1 + differentBitAtPrefixIndex + 1,
					&(e->values_));
			newSubnode->suffixes_[newSubnodeEntryHCAddress] = e->values_;
		}
	} else if (filled_[hcAddress] && !hasSubnode_[hcAddress]) {
		cout << "create subnode with existing suffix" << endl;
		// node entry and suffix exist:
		// convert suffix to new node with prefix (longest common) + insert
		AHC* subnode = new AHC(dim_, valueLength_);
		subnodes_[hcAddress] = subnode;
		hasSubnode_[hcAddress] = true;

		// set longest common prefix in subnode
		bool allDimSame = true;
		int prefixLength = 0;
		for (int i = currentIndex; i < valueLength_ && allDimSame; i++) {
			for (int val = 0; val < dim_ && allDimSame; val++)
				allDimSame = e->values_[val][i]
						== suffixes_[hcAddress][val][i - currentIndex];

			if (allDimSame)
				prefixLength++;
			for (int val = 0; val < dim_ && allDimSame; val++)
				subnode->prefix_[val].push_back(e->values_[val][i]);
		}

		// address in subnode starts after common prefix
		long insertEntryHCAddress = interleaveBits(
				currentIndex + 1 + prefixLength, e);
		long existingEntryHCAddress = interleaveBits(prefixLength,
				&suffixes_[hcAddress]);

		subnode->filled_[insertEntryHCAddress] = true;
		subnode->filled_[existingEntryHCAddress] = true;

		// add remaining bits after prefix and addresses as suffixes
		removeFirstBits(currentIndex + 1 + prefixLength + 1, &(e->values_));
		removeFirstBits(prefixLength + 1, &(suffixes_[hcAddress]));
		subnode->suffixes_[insertEntryHCAddress] = e->values_;
		subnode->suffixes_[existingEntryHCAddress] = suffixes_[hcAddress];
	} else {
		cout << "insert" << endl;
		// node entry does not exist:
		// insert entry + suffix
		filled_[hcAddress] = true;
		removeFirstBits(currentIndex + 1, &(e->values_));
		suffixes_[hcAddress] = e->values_;
	}
}

vector<vector<bool>> AHC::getSuffix(long hcAddress) {
	return suffixes_[hcAddress];
}

size_t AHC::getSuffixSize(long hcAddress) {
	if (suffixes_.size() == 0 || suffixes_[hcAddress].size() == 0)
		return 0;
	else
		return suffixes_[hcAddress][0].size();
}

void AHC::removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *values) {
	for (size_t i = 0; i < values->size(); i++) {
		(*values)[i].erase((*values)[i].begin(),
				(*values)[i].begin() + nBitsToRemove);
	}
}

void AHC::duplicateFirstBits(size_t nBitsToDuplicate, vector<vector<bool>> from,
		vector<vector<bool>>* to) {
	for (size_t i = 0; i < from.size(); i++) {
		for (size_t j = 0; j < nBitsToDuplicate; j++) {
			(*to)[i].push_back(from[i][j]);
		}
	}
}
