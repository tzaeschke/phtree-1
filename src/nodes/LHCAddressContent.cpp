/*
 * LHCAddressContent.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: max
 */

#include "LHCAddressContent.h"

LHCAddressContent::LHCAddressContent() {
	hasSubnode = false;
	subnode = ((Node *) 0);
}
LHCAddressContent::LHCAddressContent(Node* subnode) {
	hasSubnode = true;
	this->subnode = subnode;
}
LHCAddressContent::LHCAddressContent(std::vector<std::vector<bool>> s) : suffix(s) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}

LHCAddressContent::LHCAddressContent(std::vector<std::vector<bool>>* s) : suffix(*s) {
	hasSubnode = false;
	subnode = ((Node *) 0);
}
