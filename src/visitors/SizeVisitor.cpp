/*
 * SizeVisitor.cpp
 *
 *  Created on: Apr 13, 2016
 *      Author: max
 */

#include "visitors/SizeVisitor.h"
#include "nodes/Node.h"
#include "nodes/LHC.h"
#include "nodes/AHC.h"
#include "nodes/LHCAddressContent.h"

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
		totalAHCByteSize += getBoolContainerSize(node->suffixes_.at(i));
	}
}

unsigned long SizeVisitor::superSize(const Node* node) {
	unsigned long superSize = sizeof(node->dim_) + sizeof(node->valueLength_);
	superSize += getBoolContainerSize(node->prefix_);
	return superSize;
}

unsigned long SizeVisitor::getBoolContainerSize(const MultiDimBitset& container) {
	unsigned long size = sizeof(container.dim_);
	// internally maps each bool to one bit only
	size += sizeof(boost::dynamic_bitset<>) + container.bits.bits_per_block * container.bits.num_blocks() / 8;
	return size;
}

void SizeVisitor::reset() {
	totalLHCByteSize = 0;
	totalAHCByteSize = 0;
}

unsigned long SizeVisitor::getTotalByteSize() const {
	return totalLHCByteSize + totalAHCByteSize;
}

unsigned long SizeVisitor::getTotalKByteSize() const {
	return getTotalByteSize() / 1000;
}

unsigned long SizeVisitor::getTotalMByteSize() const {
	return getTotalByteSize() / 1000000;
}

unsigned long SizeVisitor::getTotalLhcByteSize() const {
	return totalLHCByteSize;
}

unsigned long SizeVisitor::getTotalLhcKByteSize() const {
	return getTotalLhcByteSize() / 1000;
}

unsigned long SizeVisitor::getTotalLhcMByteSize() const {
	return getTotalLhcByteSize() / 1000000;
}

unsigned long SizeVisitor::getTotalAhcByteSize() const {
	return totalAHCByteSize;
}

unsigned long SizeVisitor::getTotalAhcKByteSize() const {
	return getTotalAhcByteSize() / 1000;
}

unsigned long SizeVisitor::getTotalAhcMByteSize() const {
	return getTotalAhcByteSize() / 1000000;
}
