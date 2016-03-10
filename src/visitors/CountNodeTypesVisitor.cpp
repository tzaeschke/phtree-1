/*
 * CountNodeTypesVisitor.cpp
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#include "CountNodeTypesVisitor.h"

using namespace std;

CountNodeTypesVisitor::CountNodeTypesVisitor() {
	reset();
}

CountNodeTypesVisitor::~CountNodeTypesVisitor() {
}

void CountNodeTypesVisitor::reset() {
	nAHCNodes_ = 0;
	nLHCNodes_ = 0;
}

unsigned long CountNodeTypesVisitor::getNumberOfVisitedAHCNodes() {
	return nAHCNodes_;
}

unsigned long CountNodeTypesVisitor::getNumberOfVisitedLHCNodes() {
	return nLHCNodes_;
}

void CountNodeTypesVisitor::visit(LHC* node) {
	nLHCNodes_++;
}

void CountNodeTypesVisitor::visit(AHC* node) {
	nAHCNodes_++;
}

ostream& operator <<(ostream &out, const CountNodeTypesVisitor &visitor) {
	out << "nodes: " << (visitor.nAHCNodes_ + visitor.nLHCNodes_);
	out << " (AHC nodes: " << visitor.nAHCNodes_;
	out << " | LHC nodes: " << visitor.nLHCNodes_  << ")"<< endl;
	return out;
}
