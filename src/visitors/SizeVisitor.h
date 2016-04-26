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

class Node;

class SizeVisitor: public Visitor {
	friend std::ostream& operator <<(std::ostream &out, const SizeVisitor& v);
public:
	SizeVisitor();
	virtual ~SizeVisitor();

	virtual void visit(LHC* node, unsigned int depth) override;
	virtual void visit(AHC* node, unsigned int depth) override;
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
	unsigned long superSize(const Node* node);
	unsigned long getBoolContainerSize(const MultiDimBitset& container);
};

#endif /* SRC_VISITORS_SIZEVISITOR_H_ */
