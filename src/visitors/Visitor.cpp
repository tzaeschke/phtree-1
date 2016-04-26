/*
 * Visitor.cpp
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#include "visitors/Visitor.h"

Visitor::~Visitor() {
}

std::ostream& Visitor::operator <<(std::ostream &out) {
	return output(out);
}
