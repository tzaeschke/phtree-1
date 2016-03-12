/*
 * AssertionVisitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_ASSERTIONVISITOR_H_
#define VISITORS_ASSERTIONVISITOR_H_

#include "Visitor.h"

class AssertionVisitor: public Visitor {
public:
	AssertionVisitor();
	virtual ~AssertionVisitor();

	virtual void visit(LHC* node, unsigned int depth) override;
	virtual void visit(AHC* node, unsigned int depth) override;
	virtual void reset() override;
};

#endif /* VISITORS_ASSERTIONVISITOR_H_ */
