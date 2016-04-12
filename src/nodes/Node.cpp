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
#include "../util/SpatialSelectionOperationsUtil.h"
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

RangeQueryIterator* Node::rangeQuery(const Entry* lowerLeft, const Entry* upperRight, size_t depth, size_t index) {
	vector<Node*>* visitedNodes = new vector<Node*>();
	SpatialSelectionOperationsUtil::lookup(lowerLeft, this, visitedNodes);
	RangeQueryIterator* iterator = new RangeQueryIterator(visitedNodes, dim_, valueLength_, lowerLeft, upperRight);
	return iterator;
}

size_t Node::getSuffixSize(NodeAddressContent content) {
	if (content.hasSubnode) {
		return 0;
	} else {
		return content.suffix->size() / dim_;
	}
}

size_t Node::getPrefixLength() {
	return prefix_.size() / dim_;
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
