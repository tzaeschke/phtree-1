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

using namespace std;

SizeVisitor::SizeVisitor() {
	reset();
}

SizeVisitor::~SizeVisitor() { }

void SizeVisitor::visit(LHC* node, unsigned int depth) {
	totalLHCByteSize += superSize(node);
	totalLHCByteSize += sizeof(node->longestSuffix_);
	totalLHCByteSize += sizeof(map<long,LHCAddressContent>)
			+ (sizeof(long) + sizeof(LHCAddressContent)) * node->sortedContents_.size();
}

void SizeVisitor::visit(AHC* node, unsigned int depth) {
	totalAHCByteSize += superSize(node);
	for (size_t i = 0; i < node->contents_.size(); ++i) {
		totalAHCByteSize += sizeof(node->contents_[i]);
		totalAHCByteSize += getBoolContainerSize(node->suffixes_[i]);
	}
}

unsigned long SizeVisitor::superSize(Node* node) {
	unsigned long superSize = sizeof(node->dim_) + sizeof(node->valueLength_);
	superSize += getBoolContainerSize(node->prefix_);
	return superSize;
}

unsigned long SizeVisitor::getBoolContainerSize(const vector<bool>& container) {
	// internally maps each bool to one bit only
	return sizeof(vector<bool>) + container.size() / 8;
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
