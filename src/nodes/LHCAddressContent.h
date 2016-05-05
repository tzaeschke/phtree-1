/*
 * LHCAddressContent.h
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#ifndef SRC_NODES_LHCADDRESSCONTENT_H_
#define SRC_NODES_LHCADDRESSCONTENT_H_

#include "util/MultiDimBitset.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
struct LHCAddressContent {
	bool hasSubnode;
	int id;
	union {
		Node<DIM>* subnode;
		unsigned long* suffixStartBlock;
	};

	LHCAddressContent();
	LHCAddressContent(const LHCAddressContent<DIM> &other);
	LHCAddressContent(Node<DIM>* subnode);
	LHCAddressContent(const unsigned long* suffixStartBlock, int id);
};

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent() :
		hasSubnode(false), id(0), subnode((Node<DIM> *) 0) {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(const LHCAddressContent<DIM> &other) :
		hasSubnode(other.hasSubnode),  id(other.id), subnode(other.subnode) {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(Node<DIM>* sub) :
		hasSubnode(true), id(0), subnode(sub) {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(const unsigned long* s, int i) :
		hasSubnode(false), id(i), subnode((Node<DIM> *) 0), suffixStartBlock(s) {
}

#endif /* SRC_NODES_LHCADDRESSCONTENT_H_ */
