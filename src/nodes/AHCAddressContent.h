/*
 * AHCAddressContent.h
 *
 *  Created on: Apr 14, 2016
 *      Author: max
 */

#ifndef SRC_NODES_AHCADDRESSCONTENT_H_
#define SRC_NODES_AHCADDRESSCONTENT_H_

#include <vector>

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
struct AHCAddressContent {
	AHCAddressContent();
	AHCAddressContent(Node<DIM>* subnode);
	AHCAddressContent(unsigned long* suffixStartBlock, int id);

	int id;
	union {
		Node<DIM>* subnode;
		unsigned long* suffixStartBlock;
	};
};

#include "nodes/Node.h"
using namespace std;

template <unsigned int DIM>
AHCAddressContent<DIM>::AHCAddressContent() : id(0) {

}

template <unsigned int DIM>
AHCAddressContent<DIM>::AHCAddressContent(Node<DIM>* sub) : id(0), subnode(sub) {
}

template <unsigned int DIM>
AHCAddressContent<DIM>::AHCAddressContent(unsigned long* s, int i) :
	id(i), suffixStartBlock(s) {
}

#endif /* SRC_NODES_AHCADDRESSCONTENT_H_ */
