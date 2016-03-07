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
#include "NodeIterator.h"
#include "NodeAddressContent.h"

class Node {
	friend class NodeIterator;
	friend std::ostream& operator<<(std::ostream& os, Node& node);

public:

	Node(size_t dim, size_t valueLength);
	Node(Node* other);
	virtual ~Node();
	Node* insert(Entry* e, size_t depth, size_t index);
	bool lookup(Entry* e, size_t depth, size_t index);

	virtual std::ostream& output(std::ostream& os, size_t depth) = 0;
	virtual NodeIterator* begin() = 0;
	virtual NodeIterator* end() = 0;


protected:
	size_t dim_;
	size_t valueLength_;
	// value -> bit
	std::vector<std::vector<bool>> prefix_;

	size_t getSuffixSize(NodeAddressContent*);
	size_t getPrefixLength();
	long interleaveBits(size_t index, Entry* e);
	long interleaveBits(size_t index, std::vector<std::vector<bool> >* values);

	virtual NodeAddressContent* lookup(long address) = 0;
	virtual void insertAtAddress(long hcAddress, std::vector<std::vector<bool>>* suffix) = 0;
	virtual void insertAtAddress(long hcAddress, Node* subnode) = 0;
	virtual Node* adjustSize() = 0;

private:
	void removeFirstBits(size_t nBitsToRemove, std::vector<std::vector<bool>> *values);
	void removeFirstBits(size_t nBitsToRemove, std::vector<std::vector<bool>> *valuesFrom, std::vector<std::vector<bool>>* valuesTo);
	void duplicateFirstBits(size_t nBitsToDuplicate, std::vector<std::vector<bool>>* from, std::vector<std::vector<bool>>* to);
	Node* determineNodeType(size_t dim, size_t valueLength, size_t nDirectInserts);
	size_t setLongestCommonPrefix(Node* nodeToSetTo, size_t startIndex, std::vector<std::vector<bool>>* entry1, std::vector<std::vector<bool>>* entry2);
};

#endif /* SRC_NODE_H_ */
