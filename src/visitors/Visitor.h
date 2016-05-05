/*
 * Visitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_VISITOR_H_
#define VISITORS_VISITOR_H_

#include <iostream>

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class LHC;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class AHC;

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class Visitor {
public:
	Visitor();
	virtual ~Visitor();

	virtual void visit(LHC<DIM, PREF_BLOCKS>* node, unsigned int depth) =0;
	virtual void visit(AHC<DIM, PREF_BLOCKS>* node, unsigned int depth) =0;
	virtual void reset() =0;
	std::ostream& operator <<(std::ostream &out);

protected:
	virtual std::ostream& output(std::ostream &out) const =0;
};

template <unsigned int DIM, unsigned int PREF_BLOCKS>
Visitor<DIM, PREF_BLOCKS>::Visitor() {
}


template <unsigned int DIM, unsigned int PREF_BLOCKS>
Visitor<DIM, PREF_BLOCKS>::~Visitor() {
}

template <unsigned int DIM, unsigned int PREF_BLOCKS>
std::ostream& Visitor<DIM, PREF_BLOCKS>::operator <<(std::ostream &out) {
	return output(out);
}


#endif /* VISITORS_VISITOR_H_ */
