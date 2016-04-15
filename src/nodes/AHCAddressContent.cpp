/*
 * AHCAddressContent.cpp
 *
 *  Created on: Apr 14, 2016
 *      Author: max
 */

#include "nodes/AHCAddressContent.h"
#include "nodes/Node.h"

using namespace std;

AHCAddressContent::AHCAddressContent() :
		filled(false), hasSubnode(false) {

}

AHCAddressContent::AHCAddressContent(Node* sub) :
		filled(true), hasSubnode(true), subnode(sub) {
}

AHCAddressContent::AHCAddressContent(int i) :
		filled(true), hasSubnode(false), id(i) {
}
