/*
 * Visitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_VISITOR_H_
#define VISITORS_VISITOR_H_

class LHC;
class AHC;

class Visitor {
public:
	Visitor();
	virtual ~Visitor();

	virtual void visit(LHC* node, unsigned int depth) =0;
	virtual void visit(AHC* node, unsigned int depth) =0;
	virtual void reset() =0;
};

#endif /* VISITORS_VISITOR_H_ */
