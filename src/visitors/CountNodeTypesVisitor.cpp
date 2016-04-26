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

std::ostream& operator <<(std::ostream &out, const CountNodeTypesVisitor& v) {
	return v.output(out);
}

std::ostream& CountNodeTypesVisitor::output(std::ostream &out) const {
	out << "nodes: " << (getNumberOfVisitedAHCNodes() + getNumberOfVisitedLHCNodes());
	out << " (AHC nodes: " << getNumberOfVisitedAHCNodes();
	out << " | LHC nodes: " << getNumberOfVisitedLHCNodes()  << ")"<< std::endl;
	return out;
}
