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
	virtual NodeIterator<DIM>* begin() const = 0;
	virtual NodeIterator<DIM>* end() const = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) =0;
	virtual void recursiveDelete() = 0;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;
	virtual size_t getMaximumNumberOfContents() const = 0;
	virtual size_t getPrefixLength() const =0;
	virtual unsigned long* getPrefixStartBlock() =0;
	virtual const unsigned long* getFixPrefixStartBlock() const =0;
	virtual void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) const = 0;
	virtual NodeAddressContent<DIM> lookup(unsigned long address) const =0;
	virtual void insertAtAddress(unsigned long hcAddress, const unsigned long* const startSuffixBlock, int id) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, unsigned long startSuffixBlock, int id) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) = 0;
	virtual Node<DIM>* adjustSize() = 0;
	virtual bool canStoreSuffixInternally(size_t nSuffixBits) =0;
};

#endif /* SRC_NODE_H_ */
