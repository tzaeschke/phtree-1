/*
 * LHCAddressContent.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#include "nodes/LHCAddressContent.h"

LHCAddressContent::LHCAddressContent() : id(0) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}
LHCAddressContent::LHCAddressContent(Node* subnode) : id(0) {
	hasSubnode = true;
	this->subnode = subnode;
}
LHCAddressContent::LHCAddressContent(boost::dynamic_bitset<> s, int i) : suffix(s), id(i) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}

LHCAddressContent::LHCAddressContent(boost::dynamic_bitset<>* s, int i) : suffix(*s), id(i) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}
