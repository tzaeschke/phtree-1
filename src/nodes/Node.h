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
class RangeQueryIterator;
template <unsigned int DIM>
class SizeVisitor;
template <unsigned int DIM>
class PrefixSharingVisitor;
template <unsigned int DIM>
class DynamicNodeOperationsUtil;

template <unsigned int DIM>
class Node {
	// TODO remove friends and use getters and setters
	friend class NodeIterator<DIM>;
	template <unsigned int D>
	friend std::ostream& operator<<(std::ostream& os, Node<D>& node);
	friend class RangeQueryIterator<DIM>;
	friend class DynamicNodeOperationsUtil<DIM>;
	friend class SpatialSelectionOperationsUtil;
	friend class SizeVisitor<DIM>;
	friend class PrefixSharingVisitor<DIM>;
public:

	virtual ~Node() {};
	virtual RangeQueryIterator<DIM>* rangeQuery(const Entry<DIM>* lowerLeft, const Entry<DIM>* upperRight, size_t depth, size_t index) =0;

	virtual std::ostream& output(std::ostream& os, size_t depth) = 0;
	virtual NodeIterator<DIM>* begin() = 0;
	virtual NodeIterator<DIM>* end() = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth) =0;
	virtual void recursiveDelete() = 0;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;

protected:
	virtual size_t getSuffixSize(NodeAddressContent<DIM>) const =0;
	virtual size_t getPrefixLength() const =0;


	virtual void lookup(unsigned long address, NodeAddressContent<DIM>& outContent) = 0;
	virtual NodeAddressContent<DIM> lookup(unsigned long address) =0;
	virtual void insertAtAddress(unsigned long hcAddress, unsigned long* startSuffixBlock, int id) = 0;
	virtual void insertAtAddress(unsigned long hcAddress, Node<DIM>* subnode) = 0;
	virtual Node<DIM>* adjustSize() = 0;
};

template <unsigned int D>
ostream& operator <<(ostream& os, Node<D> &node) {
	return node.output(os, 1);
}

#endif /* SRC_NODE_H_ */
