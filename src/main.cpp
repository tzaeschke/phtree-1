#include <iostream>
#include <string>
#include <vector>
#include <bitset>

#define BIT_LENGTH 8
using namespace std;

class Entry {
public:
	Entry(vector<int> values, int bitLength);
	~Entry() {
		delete &values_;
	}
	// value -> bit
	vector<vector<bool>> values_;

	friend ostream& operator << (ostream &out, const Entry &entry);
};

ostream& operator << (ostream& os, const Entry &e)
{
	os << "Entry: (";
	for (size_t value = 0; value < e.values_.size(); value++) {
		os << "(";
		for (size_t bit = 0; bit < e.values_[value].size(); bit++) {
			int bitNumber = (e.values_[value][bit]) ? 1 : 0;
			os << bitNumber;
		}

		os << "), ";
	}
	os << ")";

	return os;
}

Entry::Entry(vector<int> values, int bitLength)
{
	values_.reserve(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		vector<bool> value(bitLength);
		for (int j = 0; j < bitLength; j++) {
			// extract j-th least segnificant bit from int
			int lsbIndex = bitLength - j - 1;
			bool bit = ((values[i] & (1 << lsbIndex)) >> lsbIndex) == 1;
			value[j] = bit;
		}
		values_.push_back(value);
	}
}

class Node {
public:
	// value -> bit
	vector<vector<bool>>  prefix_;

	virtual ~Node() {
		delete &prefix_;
	}

	virtual void insert(Entry* e, int depth, int index) {
	}

	virtual bool lookup(Entry* e, int depth, int index) {
		return false;
	}

	virtual size_t getSuffixSize(long hcAddress) {
		return 0;
	}

	size_t getPrefixLength() {
		if (prefix_.size() == 0) return 0;
		else return prefix_[0].size();
	}

	long interleaveBits(int index, Entry* e) {
		return interleaveBits(index, &(e->values_));
	}

	long interleaveBits(int index, vector<vector<bool> >* values) {
		long hcAddress = 0;
		int max = values->size() - 1;
		for (size_t value = 0; value < values->size(); value++) {
			hcAddress |= (*values)[value][index] << (max - value);
		}
		return hcAddress;
	}
};

class AHC : public Node {
public:
	AHC(int dim, int valueLength);
	~AHC() {
		delete &hasSubnode_;
		delete &filled_;
		for (size_t i = 0; i < subnodes_.size(); i++) {
			delete subnodes_[i];
		}
		delete &subnodes_;
		delete &suffixes_;
	}

	bool lookup(Entry* e, int depth, int index) {
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
		}
		else {
			for (size_t bit = 0; bit < getSuffixSize(hcAddress); bit++) {
				for (size_t value = 0; value < e->values_.size(); value++) {
					if (e->values_[value][currentIndex + 1 + bit] != suffixes_[hcAddress][value][bit]) {
						cout << "suffix missmatch" << endl;
						return false;
					}
				}
			}

			cout << "found" << endl;
			return true;
		}
	}

	void insert(Entry* e, int depth, int index) {
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
					prefixIncluded = e->values_[value][currentIndex + 1 + i] == subnodes_[hcAddress]->prefix_[value][i];
					if (!prefixIncluded) differentBitAtPrefixIndex = i;
				}
			}

			if (prefixIncluded) {
				// recurse on subnode
				cout << "recurse" << endl;
				subnodes_[hcAddress]->insert(e, depth + 1, currentIndex + 1);
			}
			else {
				cout << "split subnode prefix" << endl;
				// split prefix of subnode [A | d | B] where d is the index of the first different bit
				// create new node with prefix A and only leave prefix B in old subnode
				Node* oldSubnode = subnodes_[hcAddress];
				AHC* newSubnode = new AHC(dim_, valueLength_);

				long newSubnodeEntryHCAddress = interleaveBits(currentIndex + 1 + differentBitAtPrefixIndex, e);
				long newSubnodePrefixDiffHCAddress = interleaveBits(differentBitAtPrefixIndex, &(oldSubnode->prefix_));

				newSubnode->filled_[newSubnodeEntryHCAddress] = true;
				newSubnode->filled_[newSubnodePrefixDiffHCAddress] = true;
				newSubnode->hasSubnode_[newSubnodePrefixDiffHCAddress] = true;
				subnodes_[hcAddress] = newSubnode;

				// move A part of old prefix to new subnode and remove [A | d] from old prefix
				duplicateFirstBits(differentBitAtPrefixIndex, oldSubnode->prefix_, &(newSubnode->prefix_));
				removeFirstBits(differentBitAtPrefixIndex + 1, &(oldSubnode->prefix_));

				removeFirstBits(currentIndex + 1 + differentBitAtPrefixIndex + 1, &(e->values_));
				newSubnode->suffixes_[newSubnodeEntryHCAddress] = e->values_;
			}
		}
		else if (filled_[hcAddress] && !hasSubnode_[hcAddress]) {
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
					allDimSame = e->values_[val][i] == suffixes_[hcAddress][val][i - currentIndex];

				if (allDimSame) prefixLength++;
				for (int val = 0; val < dim_ && allDimSame; val++)
					subnode->prefix_[val].push_back(e->values_[val][i]);
			}

			// address in subnode starts after common prefix
			long insertEntryHCAddress = interleaveBits(currentIndex + 1 + prefixLength, e);
			long existingEntryHCAddress = interleaveBits(prefixLength, &suffixes_[hcAddress]);

			subnode->filled_[insertEntryHCAddress] = true;
			subnode->filled_[existingEntryHCAddress] = true;

			// add remaining bits after prefix and addresses as suffixes
			removeFirstBits(currentIndex + 1 + prefixLength + 1, &(e->values_));
			removeFirstBits(prefixLength + 1, &(suffixes_[hcAddress]));
			subnode->suffixes_[insertEntryHCAddress] = e->values_;
			subnode->suffixes_[existingEntryHCAddress] = suffixes_[hcAddress];
		}
		else {
			cout << "insert" << endl;
			// node entry does not exist:
			// insert entry + suffix
			filled_[hcAddress] = true;
			removeFirstBits(currentIndex + 1, &(e->values_));
			suffixes_[hcAddress] = e->values_;
		}
	}

	vector<vector<bool>> getSuffix(long hcAddress) {
		return suffixes_[hcAddress];
	}

	size_t getSuffixSize(long hcAddress) {
		if (suffixes_.size() == 0 || suffixes_[hcAddress].size() == 0) return 0;
		else return suffixes_[hcAddress][0].size();
	}

private:
	int dim_;
	int valueLength_;
	vector<bool> filled_;
	vector<bool> hasSubnode_;
	vector<Node *> subnodes_;
	// entry -> value -> bit
	vector<vector<vector<bool>>> suffixes_;

	void removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *values) {
		for (size_t i = 0; i < values->size(); i++) {
			(*values)[i].erase((*values)[i].begin(), (*values)[i].begin() + nBitsToRemove);
		}
	}

	void duplicateFirstBits(size_t nBitsToDuplicate, vector<vector<bool>> from, vector<vector<bool>>* to) {
		for (size_t i = 0; i < from.size(); i++) {
			for (size_t j = 0; j < nBitsToDuplicate; j++) {
				(*to)[i].push_back(from[i][j]);
			}
		}
	}
};

AHC::AHC(int dim, int valueLength)
{
	valueLength_ = valueLength;
	dim_ = dim;
	long maxElements = 1 << dim;
	filled_ = vector<bool>(maxElements, false);
	hasSubnode_ = vector<bool>(maxElements, false);
	subnodes_ = vector<Node*>(maxElements);
	suffixes_ = vector<vector<vector<bool>>>(maxElements);
	prefix_ = vector<vector<bool>>(dim_);
}

class PHTree {
public:
	PHTree(int dim, int valueLength);
	~PHTree() {
		delete root_;
	}

	Node* root_;
	int dim_; // dimensions (k)
	int valueLength_;

	void insert(Entry* e) {
		cout << "inserting: " << *e << endl;
		root_->insert(e, 0, 0);
	}

	bool lookup(Entry* e) {
		cout << "searching: " << *e << endl;
		return root_->lookup(e, 0, 0);
	}
};


PHTree::PHTree(int dim, int valueLength)
{
	valueLength_ = valueLength;
	dim_ = dim;
	root_ = new AHC(dim, valueLength);
}


int main() {

	// TODO: restructure project with header files
	// TODO: restructure constructors into classes
	// TODO: duplicate entry before removing bits to store them as a suffix so the original object is preserved
	// TODO: add << for PH tree class
	// TODO: use algorithm pattern to elevate functionality into Node class
	// TODO: add LHC
	// TODO: add range query

	vector<int> e1Values { 10, 5 };
	vector<int> e2Values { 11, 12 };
	vector<int> e3Values{ 60, 16 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);
	Entry* e1Duplicate = new Entry(e1Values, BIT_LENGTH);
	Entry* e3Duplicate = new Entry(e3Values, BIT_LENGTH);

	PHTree* phtree = new PHTree(2, BIT_LENGTH);
	phtree->insert(e1);

	phtree->insert(e2);
	phtree->lookup(e1Duplicate);
	phtree->lookup(e3Duplicate);

	phtree->insert(e3);
	phtree->lookup(e3Duplicate);

	delete phtree;
	delete e1;
	delete e2;


	return 0;
};
