/*
 * LHCAddressContent.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#include "nodes/LHCAddressContent.h"

LHCAddressContent::LHCAddressContent() :
		hasSubnode(false), id(0), subnode((Node *) 0) {
}

LHCAddressContent::LHCAddressContent(const LHCAddressContent &other) :
		hasSubnode(other.hasSubnode),  id(other.id), subnode(other.subnode), suffix(other.suffix) {
}

LHCAddressContent::LHCAddressContent(Node* sub) :
		hasSubnode(true), id(0), subnode(sub) {
}

LHCAddressContent::LHCAddressContent(const MultiDimBitset* s, int i) :
		hasSubnode(false), id(i), subnode((Node *) 0), suffix(*s) {
}

LHCAddressContent::LHCAddressContent(const size_t dim, int i) :
		hasSubnode(false), id(i), subnode((Node *) 0), suffix(dim) {
}
