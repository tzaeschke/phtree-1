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
	AHCAddressContent(int id);

	bool filled;
	bool hasSubnode;
	union {
		Node<DIM>* subnode;
		int id;
	};
};

#include "nodes/Node.h"
using namespace std;

template <unsigned int DIM>
AHCAddressContent<DIM>::AHCAddressContent() :
		filled(false), hasSubnode(false) {

}

template <unsigned int DIM>
AHCAddressContent<DIM>::AHCAddressContent(Node<DIM>* sub) :
		filled(true), hasSubnode(true), subnode(sub) {
}

template <unsigned int DIM>
AHCAddressContent<DIM>::AHCAddressContent(int i) :
		filled(true), hasSubnode(false), id(i) {
}

#endif /* SRC_NODES_AHCADDRESSCONTENT_H_ */
