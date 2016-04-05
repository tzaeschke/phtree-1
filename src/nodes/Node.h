/*
 * Node.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include <vector>
#include "../Entry.h"
#include "../iterators/NodeIterator.h"
#include "NodeAddressContent.h"

class Visitor;
class RangeQueryIterator;

class Node {
	// TODO remove friends and use getters and setters
	friend class NodeIterator;
	friend std::ostream& operator<<(std::ostream& os, Node& node);
	friend class RangeQueryIterator;

public:

	Node(size_t dim, size_t valueLength);
	Node(Node* other);
	virtual ~Node();
	Node* insert(Entry* e, size_t depth, size_t index);
	bool lookup(Entry* e, size_t depth, size_t index, std::vector<Node*>* visitedNodes);
	RangeQueryIterator* rangeQuery(Entry* lowerLeft, Entry* upperRight, size_t depth, size_t index);

	virtual std::ostream& output(std::ostream& os, size_t depth) = 0;
	virtual NodeIterator* begin() = 0;
	virtual NodeIterator* end() = 0;
	virtual void accept(Visitor* visitor, size_t depth);
	virtual void recursiveDelete() = 0;

protected:
	size_t dim_;
	size_t valueLength_;
	// value -> bit
	std::vector<std::vector<bool>> prefix_;

	size_t getSuffixSize(NodeAddressContent);
	size_t getPrefixLength();


	virtual NodeAddressContent lookup(unsigned long address) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, std::vector<std::vector<bool>>* suffix) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, Node* subnode) = 0;
	virtual Node* adjustSize() = 0;


private:
	Node* determineNodeType(size_t dim, size_t valueLength, size_t nDirectInserts);
};

#endif /* SRC_NODE_H_ */
