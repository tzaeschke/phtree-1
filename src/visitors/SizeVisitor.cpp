/*
 * SizeVisitor.cpp
 *
 *  Created on: Apr 13, 2016
 *      Author: max
 */

#include "SizeVisitor.h"
#include "../nodes/Node.h"
#include "../nodes/LHC.h"
#include "../nodes/AHC.h"
#include "../nodes/LHCAddressContent.h"

SizeVisitor::SizeVisitor() {
	reset();
}

SizeVisitor::~SizeVisitor() { }

void SizeVisitor::visit(LHC* node, unsigned int depth) {
	totalLHCByteSize += superSize(node);
	totalLHCByteSize += sizeof(node->longestSuffix_);
	totalLHCByteSize += sizeof(node->sortedContents_);
	for ( std::map<long,LHCAddressContent>::iterator i = node->sortedContents_.begin();
			i != node->sortedContents_.end() ; ++i ) {
		totalLHCByteSize += sizeof(i->first);
		totalLHCByteSize += sizeof(i->second);
	}
}

void SizeVisitor::visit(AHC* node, unsigned int depth) {
	totalAHCByteSize += superSize(node);
	totalAHCByteSize += sizeof(node->filled_);
	totalAHCByteSize += sizeof(node->hasSubnode_);
	totalAHCByteSize += sizeof((*node->ids_));
	totalAHCByteSize += sizeof(node->subnodes_);
	totalAHCByteSize += sizeof(node->suffixes_);
}

unsigned long SizeVisitor::superSize(Node* node) {
	unsigned long superSize = sizeof(node->dim_) + sizeof(node->valueLength_);
	superSize += sizeof(node->prefix_);
	return superSize;
}

void SizeVisitor::reset() {
	totalLHCByteSize = 0;
	totalAHCByteSize = 0;
}

unsigned long SizeVisitor::getTotalByteSize() {
	return totalLHCByteSize + totalAHCByteSize;
}

unsigned long SizeVisitor::getTotalKByteSize() {
	return getTotalByteSize() / 1000;
}

unsigned long SizeVisitor::getTotalMByteSize() {
	return getTotalByteSize() / 1000000;
}

unsigned long SizeVisitor::getTotalLhcByteSize() {
	return totalLHCByteSize;
}

unsigned long SizeVisitor::getTotalLhcKByteSize() {
	return getTotalLhcByteSize() / 1000;
}

unsigned long SizeVisitor::getTotalLhcMByteSize() {
	return getTotalLhcByteSize() / 1000000;
}

unsigned long SizeVisitor::getTotalAhcByteSize() {
	return totalAHCByteSize;
}

unsigned long SizeVisitor::getTotalAhcKByteSize() {
	return getTotalAhcByteSize() / 1000;
}

unsigned long SizeVisitor::getTotalAhcMByteSize() {
	return getTotalAhcByteSize() / 1000000;
}
