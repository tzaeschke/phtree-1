/*
 * Visitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_VISITOR_H_
#define VISITORS_VISITOR_H_

#include <iostream>

template <unsigned int DIM>
class LHC;

template <unsigned int DIM>
class AHC;

template <unsigned int DIM>
class Visitor {
public:
	Visitor();
	virtual ~Visitor();

	virtual void visit(LHC<DIM>* node, unsigned int depth) =0;
	virtual void visit(AHC<DIM>* node, unsigned int depth) =0;
	virtual void reset() =0;
	std::ostream& operator <<(std::ostream &out);

protected:
	virtual std::ostream& output(std::ostream &out) const =0;
};

template <unsigned int DIM>
Visitor<DIM>::Visitor() {
}


template <unsigned int DIM>
Visitor<DIM>::~Visitor() {
}

template <unsigned int DIM>
std::ostream& Visitor<DIM>::operator <<(std::ostream &out) {
	return output(out);
}


#endif /* VISITORS_VISITOR_H_ */
