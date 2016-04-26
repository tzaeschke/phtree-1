/*
 * Visitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_VISITOR_H_
#define VISITORS_VISITOR_H_

#include <iostream>

class LHC;
class AHC;

class Visitor {
public:
	virtual ~Visitor();

	virtual void visit(LHC* node, unsigned int depth) =0;
	virtual void visit(AHC* node, unsigned int depth) =0;
	virtual void reset() =0;
	std::ostream& operator <<(std::ostream &out);

protected:
	virtual std::ostream& output(std::ostream &out) const =0;
};

#endif /* VISITORS_VISITOR_H_ */
