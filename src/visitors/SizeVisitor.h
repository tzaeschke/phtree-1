/*
 * SizeVisitor.h
 *
 *  Created on: Apr 13, 2016
 *      Author: max
 */

#ifndef SRC_VISITORS_SIZEVISITOR_H_
#define SRC_VISITORS_SIZEVISITOR_H_

#include "visitors/Visitor.h"
#include "util/MultiDimBitset.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class SizeVisitor: public Visitor<DIM> {
	template <unsigned int D>
	friend std::ostream& operator <<(std::ostream &out, const SizeVisitor<D>& v);
public:
	SizeVisitor();
	virtual ~SizeVisitor();

	virtual void visit(LHC<DIM>* node, unsigned int depth) override;
	virtual void visit(AHC<DIM>* node, unsigned int depth) override;
	virtual void reset() override;

	unsigned long getTotalBitSize() const;
	unsigned long getTotalByteSize() const;
	unsigned long getTotalKByteSize() const;
	unsigned long getTotalMByteSize() const;
	unsigned long getTotalLhcBitSize() const;
	unsigned long getTotalLhcByteSize() const;
	unsigned long getTotalLhcKByteSize() const;
	unsigned long getTotalLhcMByteSize() const;
	unsigned long getTotalAhcBitSize() const;
	unsigned long getTotalAhcByteSize() const;
	unsigned long getTotalAhcKByteSize() const;
	unsigned long getTotalAhcMByteSize() const;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	unsigned long totalLHCByteSize;
	unsigned long totalAHCByteSize;
	unsigned long superSize(const Node<DIM>* node);
	unsigned long getBoolContainerSize(const MultiDimBitset<DIM>& container);
};

#include "visitors/SizeVisitor.h"
#include "nodes/Node.h"
#include "nodes/LHC.h"
#include "nodes/AHC.h"
#include "nodes/LHCAddressContent.h"

using namespace std;

template <unsigned int DIM>
SizeVisitor<DIM>::SizeVisitor() : Visitor<DIM>() {
	reset();
}

template <unsigned int DIM>
SizeVisitor<DIM>::~SizeVisitor() { }

template <unsigned int DIM>
void SizeVisitor<DIM>::visit(LHC<DIM>* node, unsigned int depth) {
	totalLHCByteSize += superSize(node);
	totalLHCByteSize += sizeof(node->longestSuffix_);
	totalLHCByteSize += sizeof(map<long,LHCAddressContent<DIM>>)
			+ (sizeof(long) + sizeof(LHCAddressContent<DIM>)) * node->sortedContents_.size();
}

template <unsigned int DIM>
void SizeVisitor<DIM>::visit(AHC<DIM>* node, unsigned int depth) {
	totalAHCByteSize += superSize(node);
	totalAHCByteSize += sizeof(node->contents_);

	for (const auto suffix : node->suffixes_) {
		totalAHCByteSize += getBoolContainerSize(suffix);
	}
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::superSize(const Node<DIM>* node) {
	unsigned long superSize = sizeof(node->valueLength_);
	superSize += getBoolContainerSize(node->prefix_);
	return superSize;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getBoolContainerSize(const MultiDimBitset<DIM>& container) {
	// internally maps each bool to one bit only
	unsigned long size = sizeof(boost::dynamic_bitset<>) + container.bits.bits_per_block * container.bits.num_blocks() / 8;
	return size;
}

template <unsigned int DIM>
void SizeVisitor<DIM>::reset() {
	totalLHCByteSize = 0;
	totalAHCByteSize = 0;
}

template <unsigned int DIM>
std::ostream& SizeVisitor<DIM>::output(std::ostream &out) const {
	float lhcSizePercent = float(getTotalLhcByteSize()) * 100 / float(getTotalByteSize());
	return out << "total size: " << getTotalKByteSize()
			<< "KByte | " << getTotalMByteSize()
			<< "MByte (LHC: " << lhcSizePercent << "%)" << std::endl;
}

template <unsigned int D>
std::ostream& operator <<(std::ostream &out, const SizeVisitor<D>& v) {
	return v.output(out);
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalBitSize() const {
	return getTotalByteSize() * 8;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalByteSize() const {
	return totalLHCByteSize + totalAHCByteSize;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalKByteSize() const {
	return getTotalByteSize() / 1000;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalMByteSize() const {
	return getTotalByteSize() / 1000000;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalLhcBitSize() const {
	return getTotalLhcByteSize() * 8;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalLhcByteSize() const {
	return totalLHCByteSize;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalLhcKByteSize() const {
	return getTotalLhcByteSize() / 1000;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalLhcMByteSize() const {
	return getTotalLhcByteSize() / 1000000;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalAhcBitSize() const {
	return getTotalAhcByteSize() * 8;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalAhcByteSize() const {
	return totalAHCByteSize;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalAhcKByteSize() const {
	return getTotalAhcByteSize() / 1000;
}

template <unsigned int DIM>
unsigned long SizeVisitor<DIM>::getTotalAhcMByteSize() const {
	return getTotalAhcByteSize() / 1000000;
}

#endif /* SRC_VISITORS_SIZEVISITOR_H_ */
