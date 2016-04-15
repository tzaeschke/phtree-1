/*
 * SizeVisitor.h
 *
 *  Created on: Apr 13, 2016
 *      Author: max
 */

#ifndef SRC_VISITORS_SIZEVISITOR_H_
#define SRC_VISITORS_SIZEVISITOR_H_

#include <vector>
#include "Visitor.h"
class Node;

class SizeVisitor: public Visitor {
public:
	SizeVisitor();
	virtual ~SizeVisitor();

	virtual void visit(LHC* node, unsigned int depth) override;
	virtual void visit(AHC* node, unsigned int depth) override;
	virtual void reset() override;

	unsigned long getTotalByteSize();
	unsigned long getTotalKByteSize();
	unsigned long getTotalMByteSize();
	unsigned long getTotalLhcByteSize();
	unsigned long getTotalLhcKByteSize();
	unsigned long getTotalLhcMByteSize();
	unsigned long getTotalAhcByteSize();
	unsigned long getTotalAhcKByteSize();
	unsigned long getTotalAhcMByteSize();

private:
	unsigned long totalLHCByteSize;
	unsigned long totalAHCByteSize;
	unsigned long superSize(Node* node);
	unsigned long getBoolContainerSize(const std::vector<bool>& container);
};

#endif /* SRC_VISITORS_SIZEVISITOR_H_ */
