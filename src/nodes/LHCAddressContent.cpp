/*
 * LHCAddressContent.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#include "nodes/LHCAddressContent.h"

LHCAddressContent::LHCAddressContent() :
		id(0) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}

LHCAddressContent::LHCAddressContent(const LHCAddressContent &other) :
		hasSubnode(other.hasSubnode), subnode(other.subnode), suffix(other.suffix), id(other.id) {

}

LHCAddressContent::LHCAddressContent(Node* subnode) :
		id(0) {
	hasSubnode = true;
	this->subnode = subnode;
}
LHCAddressContent::LHCAddressContent(MultiDimBitset s, int i) :
		suffix(s), id(i) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}

LHCAddressContent::LHCAddressContent(MultiDimBitset* s, int i) :
		suffix(*s), id(i) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}
