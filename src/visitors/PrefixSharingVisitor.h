/*
 * PrefixSharingVisitor.h
 *
 *  Created on: Apr 25, 2016
 *      Author: max
 */

#ifndef SRC_VISITORS_PREFIXSHARINGVISITOR_H_
#define SRC_VISITORS_PREFIXSHARINGVISITOR_H_

#include "visitors/Visitor.h"

class Node;

class PrefixSharingVisitor: public Visitor {
	friend std::ostream& operator <<(std::ostream &out, const PrefixSharingVisitor& v);
public:
	PrefixSharingVisitor();
	virtual ~PrefixSharingVisitor();

	virtual void visit(LHC* node, unsigned int depth) override;
	virtual void visit(AHC* node, unsigned int depth) override;
	virtual void reset() override;

	unsigned long getPrefixSharedBits() const;
	unsigned long getPrefixBitsWithoutSharing() const;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	void visitGeneral(const Node* node);
	unsigned long prefixSharedBits;
	unsigned long prefixBitsWithoutSharing;
};

#endif /* SRC_VISITORS_PREFIXSHARINGVISITOR_H_ */
