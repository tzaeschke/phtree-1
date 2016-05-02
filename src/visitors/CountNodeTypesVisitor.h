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

template <unsigned int DIM>
class CountNodeTypesVisitor: public Visitor<DIM> {
	template <unsigned int D>
	friend std::ostream& operator <<(std::ostream &out, const CountNodeTypesVisitor<D>& v);
public:
	CountNodeTypesVisitor();
	virtual ~CountNodeTypesVisitor();

	virtual void visit(LHC<DIM>* node, unsigned int depth) override;
	virtual void visit(AHC<DIM>* node, unsigned int depth) override;
	virtual void reset() override;
	std::ostream& operator <<(std::ostream &out) const;

	unsigned long getNumberOfVisitedAHCNodes() const;
	unsigned long getNumberOfVisitedLHCNodes() const;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	unsigned long nAHCNodes_;
	unsigned long nLHCNodes_;
};

/* implementation of template */
template <unsigned int DIM>
CountNodeTypesVisitor<DIM>::CountNodeTypesVisitor() : Visitor<DIM>() {
	reset();
}

template <unsigned int DIM>
CountNodeTypesVisitor<DIM>::~CountNodeTypesVisitor() {
}

template <unsigned int DIM>
void CountNodeTypesVisitor<DIM>::reset() {
	nAHCNodes_ = 0;
	nLHCNodes_ = 0;
}

template <unsigned int DIM>
unsigned long CountNodeTypesVisitor<DIM>::getNumberOfVisitedAHCNodes() const {
	return nAHCNodes_;
}

template <unsigned int DIM>
unsigned long CountNodeTypesVisitor<DIM>::getNumberOfVisitedLHCNodes() const {
	return nLHCNodes_;
}

template <unsigned int DIM>
void CountNodeTypesVisitor<DIM>::visit(LHC<DIM>* node, unsigned int depth) {
	nLHCNodes_++;
}

template <unsigned int DIM>
void CountNodeTypesVisitor<DIM>::visit(AHC<DIM>* node, unsigned int depth) {
	nAHCNodes_++;
}

template <unsigned int D>
std::ostream& operator <<(std::ostream &out, const CountNodeTypesVisitor<D>& v) {
	return v.output(out);
}

template <unsigned int DIM>
std::ostream& CountNodeTypesVisitor<DIM>::output(std::ostream &out) const {
	out << "nodes: " << (getNumberOfVisitedAHCNodes() + getNumberOfVisitedLHCNodes());
	out << " (AHC nodes: " << getNumberOfVisitedAHCNodes();
	out << " | LHC nodes: " << getNumberOfVisitedLHCNodes()  << ")"<< std::endl;
	return out;
}

#endif /* VISITORS_COUNTNODETYPESVISITOR_H_ */
