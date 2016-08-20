/*
 * Node.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#ifndef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
	// requires upwards conversions of shared mutex
#define BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
#endif

#include "Entry.h"
#include "iterators/NodeIterator.h"
#include "nodes/NodeAddressContent.h"
#include "util/MultiDimBitset.h"
#include <boost/thread/shared_mutex.hpp>

template <unsigned int DIM>
class Visitor;
template <unsigned int DIM>
class SizeVisitor;
template <unsigned int DIM>
class PrefixSharingVisitor;
template <unsigned int DIM, unsigned int WIDTH>
class DynamicNodeOperationsUtil;
class TSuffixStorage;

enum NodeType {
	Linear,
	Array,
	AtomicLinear
};

template <unsigned int DIM>
class Node {
	// TODO remove friends and use getters and setters
	friend class NodeIterator<DIM>;
	friend class SizeVisitor<DIM>;
	friend class PrefixSharingVisitor<DIM>;
public:

	boost::upgrade_mutex rwLock;
	bool removed;

	Node();
	virtual ~Node();
	virtual std::ostream& output(std::ostream& os, size_t depth, size_t index, size_t totalBitLength) = 0;
	virtual NodeIterator<DIM>* begin() const = 0;
	virtual NodeIterator<DIM>* it(unsigned long hcAddress) const =0;
	virtual NodeIterator<DIM>* end() const = 0;
	virtual void accept(Visitor<DIM>* visitor, size_t depth, unsigned int index) =0;
	virtual void recursiveDelete() = 0;
	virtual bool full() const;
	// gets the number of contents: #suffixes + #subnodes
	virtual size_t getNumberOfContents() const = 0;
	virtual size_t getMaximumNumberOfContents() const = 0;
	virtual size_t getMaxPrefixLength() const =0;
	virtual size_t getPrefixLength() const =0;
	virtual unsigned long* getPrefixStartBlock() =0;
	virtual const unsigned long* getFixPrefixStartBlock() const =0;
	virtual void lookup(unsigned long address, NodeAddressContent<DIM>& outContent, bool resolveSuffixIndex) const = 0;

	virtual bool insertAtAddress(unsigned long hcAddress, uintptr_t pointer) =0;
	virtual bool insertAtAddress(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) = 0;
	virtual bool insertAtAddress(unsigned long hcAddress, unsigned long suffix, int id) = 0;
	virtual bool insertAtAddress(unsigned long hcAddress, const Node<DIM>* const subnode) = 0;

	virtual void linearCopyFromOther(unsigned long hcAddress, uintptr_t pointer) =0;
	virtual void linearCopyFromOther(unsigned long hcAddress, unsigned int suffixStartBlockIndex, int id) = 0;
	virtual void linearCopyFromOther(unsigned long hcAddress, unsigned long suffix, int id) = 0;
	virtual void linearCopyFromOther(unsigned long hcAddress, const Node<DIM>* const subnode) = 0;

	virtual bool updateAddress(uintptr_t pointer, const NodeAddressContent<DIM>& prevContent) = 0;
	virtual bool updateAddress(const Node<DIM>* const subnode, const NodeAddressContent<DIM>& prevContent) = 0;
	virtual bool updateAddressToSpinlock(const NodeAddressContent<DIM>& prevContent) = 0;
	bool updateAddressToSpinlock(unsigned hcAddress, Node<DIM>* subnode);
	virtual void updateAddressFromSpinlock(unsigned long hcAddress, const Node<DIM>* const subnode) = 0;
	virtual void updateAddressFromSpinlock(unsigned long hcAddress, uintptr_t pointer) = 0;

	virtual bool isAtomic() const =0;
	virtual Node<DIM>* getParent() =0;
	virtual void setParent(Node<DIM>* parent) =0;
	virtual NodeType getType() const = 0;
	virtual string getName() const =0;
	virtual bool canStoreSuffixInternally(size_t nSuffixBits) const =0;
	virtual unsigned int canStoreSuffix(size_t nSuffixBits) const =0;
	virtual void setSuffixStorage(TSuffixStorage* suffixStorage) =0;
	virtual const TSuffixStorage* getSuffixStorage() const =0;
	virtual TSuffixStorage* getChangeableSuffixStorage() const =0;
	// returns the storage address to write to and the index to store
	virtual std::pair<unsigned long*, unsigned int> reserveSuffixSpace(size_t nSuffixBits, bool atomic) =0;
	virtual void freeSuffixSpace(size_t nSuffixBits, unsigned long* suffixStartBlock, bool atomic) =0;
	virtual void copySuffixStorageFrom(const Node<DIM>& other) =0;

	NodeAddressContent<DIM> lookup(unsigned long address, bool resolveSuffixIndex) const;
	// attention: linear checks! should be used for validation only
	bool containsId(int id) const;
	size_t getNStoredSuffixes() const;
};

using namespace std;

template <unsigned int DIM>
Node<DIM>::Node() : removed(false) {
//	pthread_rwlock_init(&rwLock, NULL);
}

template <unsigned int DIM>
Node<DIM>::~Node() {
//	pthread_rwlock_destroy(&rwLock);
}

template <unsigned int DIM>
bool Node<DIM>::full() const {
	return getNumberOfContents() == getMaximumNumberOfContents();
}

template <unsigned int DIM>
NodeAddressContent<DIM> Node<DIM>::lookup(unsigned long address, bool resolveSuffixIndex) const {
	NodeAddressContent<DIM> content;
	this->lookup(address, content, resolveSuffixIndex);
	return content;
}


template <unsigned int DIM>
bool Node<DIM>::updateAddressToSpinlock(unsigned hcAddress, Node<DIM>* subnode) {
	NodeAddressContent<DIM> content;
	content.exists = true;
	content.address = hcAddress;
	content.hasSubnode = true;
	content.hasSpecialPointer = false;
	content.subnode = subnode;
	return updateAddressToSpinlock(content);
}

template <unsigned int DIM>
bool Node<DIM>::containsId(int id) const {
	NodeIterator<DIM>* startIt = this->begin();
	NodeIterator<DIM>* endIt = this->end();
	for (; (*startIt) != (*endIt); ++(*startIt)) {
		NodeAddressContent<DIM> content = *(*startIt);
		if (content.exists && !content.hasSubnode && content.id == id) {
			return true;
		}
	}

	delete startIt;
	delete endIt;
	return false;
}

template <unsigned int DIM>
size_t Node<DIM>::getNStoredSuffixes() const {
	if (this->getNumberOfContents() != 0) {
		size_t nSuffixes = 0;
		NodeIterator<DIM>* startIt = this->begin();
		NodeIterator<DIM>* endIt = this->end();
		for (; (*startIt) != (*endIt); ++(*startIt)) {
			NodeAddressContent<DIM> content = *(*startIt);
			if (content.exists && !content.hasSubnode && !content.hasSpecialPointer) {
				++nSuffixes;
			}
		}

		delete startIt;
		delete endIt;
		return nSuffixes;
	} else {
		return 0;
	}
}

#endif /* SRC_NODE_H_ */
