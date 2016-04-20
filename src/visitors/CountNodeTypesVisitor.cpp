/*
 * CountNodeTypesVisitor.cpp
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#include "visitors/CountNodeTypesVisitor.h"

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

unsigned long CountNodeTypesVisitor::getNumberOfVisitedAHCNodes() const {
	return nAHCNodes_;
}

unsigned long CountNodeTypesVisitor::getNumberOfVisitedLHCNodes() const {
	return nLHCNodes_;
}

void CountNodeTypesVisitor::visit(LHC* node, unsigned int depth) {
	nLHCNodes_++;
}

void CountNodeTypesVisitor::visit(AHC* node, unsigned int depth) {
	nAHCNodes_++;
}

ostream& operator <<(ostream &out, const CountNodeTypesVisitor &visitor) {
	out << "nodes: " << (visitor.nAHCNodes_ + visitor.nLHCNodes_);
	out << " (AHC nodes: " << visitor.nAHCNodes_;
	out << " | LHC nodes: " << visitor.nLHCNodes_  << ")"<< endl;
	return out;
}
