/*
 * CountNodeTypesVisitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_COUNTNODETYPESVISITOR_H_
#define VISITORS_COUNTNODETYPESVISITOR_H_

#include <iostream>
#include "Visitor.h"

class CountNodeTypesVisitor: public Visitor {
	friend std::ostream& operator <<(std::ostream &out, const CountNodeTypesVisitor &visitor);
public:
	CountNodeTypesVisitor();
	virtual ~CountNodeTypesVisitor();

	virtual void visit(LHC* node);
	virtual void visit(AHC* node);
	virtual void reset();

	unsigned long getNumberOfVisitedAHCNodes();
	unsigned long getNumberOfVisitedLHCNodes();
private:
	unsigned long nAHCNodes_;
	unsigned long nLHCNodes_;
};

#endif /* VISITORS_COUNTNODETYPESVISITOR_H_ */
