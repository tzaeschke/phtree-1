/*
 * CountNodeTypesVisitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_COUNTNODETYPESVISITOR_H_
#define VISITORS_COUNTNODETYPESVISITOR_H_

#include <iostream>
#include "visitors/Visitor.h"

class CountNodeTypesVisitor: public Visitor {
	friend std::ostream& operator <<(std::ostream &out, const CountNodeTypesVisitor& v);
public:
	CountNodeTypesVisitor();
	virtual ~CountNodeTypesVisitor();

	virtual void visit(LHC* node, unsigned int depth) override;
	virtual void visit(AHC* node, unsigned int depth) override;
	virtual void reset() override;

	unsigned long getNumberOfVisitedAHCNodes() const;
	unsigned long getNumberOfVisitedLHCNodes() const;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	unsigned long nAHCNodes_;
	unsigned long nLHCNodes_;
};

#endif /* VISITORS_COUNTNODETYPESVISITOR_H_ */
