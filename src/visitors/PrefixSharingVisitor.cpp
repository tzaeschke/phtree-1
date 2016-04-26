/*
 * PrefixSharingVisitor.cpp
 *
 *  Created on: Apr 25, 2016
 *      Author: max
 */

#include <visitors/PrefixSharingVisitor.h>
#include "nodes/Node.h"

using namespace std;

PrefixSharingVisitor::PrefixSharingVisitor() {
	reset();
}

PrefixSharingVisitor::~PrefixSharingVisitor() {
}

void PrefixSharingVisitor::visit(LHC* node, unsigned int depth) {
	visitGeneral((Node*)node);
}

void PrefixSharingVisitor::visit(AHC* node, unsigned int depth) {
	visitGeneral((Node*)node);
}

void PrefixSharingVisitor::visitGeneral(const Node* node) {
	prefixSharedBits += node->prefix_.getBitLength() * node->prefix_.getDim();
	prefixBitsWithoutSharing += node->dim_ * node->prefix_.getBitLength() * node->getNumberOfContents();
}

void PrefixSharingVisitor::reset() {
	prefixBitsWithoutSharing = 0;
	prefixSharedBits = 0;
}

unsigned long PrefixSharingVisitor::getPrefixSharedBits() const {
	return prefixSharedBits;
}

unsigned long PrefixSharingVisitor::getPrefixBitsWithoutSharing() const {
	return prefixBitsWithoutSharing;
}

std::ostream& operator <<(std::ostream &out, const PrefixSharingVisitor& v) {
	return v.output(out);
}

std::ostream& PrefixSharingVisitor::output(std::ostream &out) const {
	float sharingSavedPercent = float(getPrefixSharedBits()) / float(getPrefixBitsWithoutSharing());
	return out << "total shared prefixes: " << getPrefixSharedBits() << " bits"
			<< " (only " << sharingSavedPercent << "% of otherwise "
			<< getPrefixBitsWithoutSharing() << " bits)" << endl;
}
