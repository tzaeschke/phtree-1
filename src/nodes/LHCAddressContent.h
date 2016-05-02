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
	Node<DIM>* subnode;
	MultiDimBitset<DIM> suffix;

	LHCAddressContent();
	LHCAddressContent(const LHCAddressContent<DIM> &other);
	LHCAddressContent(Node<DIM>* subnode);
	LHCAddressContent(const MultiDimBitset<DIM>* suffix, int id);
	LHCAddressContent(int id);
};

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent() :
		hasSubnode(false), id(0), subnode((Node<DIM> *) 0), suffix() {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(const LHCAddressContent<DIM> &other) :
		hasSubnode(other.hasSubnode),  id(other.id), subnode(other.subnode), suffix(other.suffix) {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(Node<DIM>* sub) :
		hasSubnode(true), id(0), subnode(sub) {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(const MultiDimBitset<DIM>* s, int i) :
		hasSubnode(false), id(i), subnode((Node<DIM> *) 0), suffix(*s) {
}

template <unsigned int DIM>
LHCAddressContent<DIM>::LHCAddressContent(int i) :
		hasSubnode(false), id(i), subnode((Node<DIM> *) 0), suffix() {
}


#endif /* SRC_NODES_LHCADDRESSCONTENT_H_ */
