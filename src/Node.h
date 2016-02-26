/*
 * Node.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include <vector>
#include "Entry.h"

using namespace std;

class Node {
public:
	Node(size_t dim, size_t valueLength);
	virtual ~Node();
	void insert(Entry* e, size_t depth, size_t index);
	bool lookup(Entry* e, size_t depth, size_t index);
	friend ostream& operator<<(ostream& os, Node& node);
	virtual ostream& output(ostream& os, size_t depth);

protected:
	size_t dim_;
	size_t valueLength_;
	// value -> bit
	vector<vector<bool>> prefix_;

	struct NodeAddressContent {
		bool contained;
		bool hasSubnode;
		Node* subnode;
		vector<vector<bool>>* suffix;
	};

	size_t getSuffixSize(long hcAddress);
	size_t getPrefixLength();
	long interleaveBits(size_t index, Entry* e);
	long interleaveBits(size_t index, vector<vector<bool> >* values);

	virtual NodeAddressContent lookup(long address);
	virtual void insertAtAddress(long hcAddress, vector<vector<bool>>* suffix);
	virtual void insertAtAddress(long hcAddress, Node* subnode);


private:
	void removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *values);
	void removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *valuesFrom, vector<vector<bool>>* valuesTo);
	void duplicateFirstBits(size_t nBitsToDuplicate, vector<vector<bool>>* from, vector<vector<bool>>* to);
	Node* determineNodeType(size_t dim, size_t valueLength, size_t nDirectInserts);
	size_t setLongestCommonPrefix(Node* nodeToSetTo, size_t startIndex, vector<vector<bool>>* entry1, vector<vector<bool>>* entry2);
};

#endif /* SRC_NODE_H_ */
