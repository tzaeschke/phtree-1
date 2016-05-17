/*
 * Node.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include "Entry.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "util/MultiDimBitset.h"

template <unsigned int DIM>
class Visitor;
template <unsigned int DIM>
class SizeVisitor;
template <unsigned int DIM>
class PrefixSharingVisitor;
template <unsigned int DIM, unsigned int WIDTH>
class DynamicNodeOperationsUtil;

template <unsigned int DIM>
class Node {
	// TODO remove friends and use getters and setters
	friend class NodeIterator<DIM>;
	friend class SizeVisitor<DIM>;
	friend class PrefixSharingVisitor<DIM>;
public:

	virtual ~Node() {};
	virtual std::ostream& output(std::ostream& os, size_t depth, size_t index, size_t totalBitLength) = 0;
	virtual NodeIterator<DIM>* begin() = 0;
	virtual NodeIterator<DIM>* end() = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) =0;
	virtual void recursiveDelete() = 0;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;
	virtual size_t getMaximumNumberOfContents() const = 0;
	virtual size_t getPrefixLength() const =0;
	virtual unsigned long* getPrefixStartBlock() =0;
	virtual void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) = 0;
	virtual NodeAddressContent<DIM> lookup(unsigned long address) =0;
	virtual void insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) = 0;
	// inserts the given subnode at the given address and returns the suffix start block
	// pointer that was at this address or null if there was none
	virtual void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) = 0;
	virtual Node<DIM>* adjustSize() = 0;
};

#endif /* SRC_NODE_H_ */
