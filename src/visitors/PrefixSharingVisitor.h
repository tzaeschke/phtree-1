/*
 * PrefixSharingVisitor.h
 *
 *  Created on: Apr 25, 2016
 *      Author: max
 */

#ifndef SRC_VISITORS_PREFIXSHARINGVISITOR_H_
#define SRC_VISITORS_PREFIXSHARINGVISITOR_H_

#include "visitors/Visitor.h"

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class PrefixSharingVisitor: public Visitor<DIM> {
	template <unsigned int D>
	friend std::ostream& operator <<(std::ostream &out, const PrefixSharingVisitor<D>& v);
public:
	PrefixSharingVisitor();
	virtual ~PrefixSharingVisitor();

	virtual void visit(LHC<DIM>* node, unsigned int depth) override;
	virtual void visit(AHC<DIM>* node, unsigned int depth) override;
	virtual void reset() override;

	unsigned long getPrefixSharedBits() const;
	unsigned long getPrefixBitsWithoutSharing() const;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	void visitGeneral(const Node<DIM>* node);
	unsigned long prefixSharedBits;
	unsigned long prefixBitsWithoutSharing;
};

#include <visitors/PrefixSharingVisitor.h>
#include "nodes/Node.h"

template <unsigned int DIM>
PrefixSharingVisitor<DIM>::PrefixSharingVisitor() : Visitor<DIM>() {
	reset();
}

template <unsigned int DIM>
PrefixSharingVisitor<DIM>::~PrefixSharingVisitor() {
}

template <unsigned int DIM>
void PrefixSharingVisitor<DIM>::visit(LHC<DIM>* node, unsigned int depth) {
	visitGeneral((Node<DIM>*)node);
}

template <unsigned int DIM>
void PrefixSharingVisitor<DIM>::visit(AHC<DIM>* node, unsigned int depth) {
	visitGeneral((Node<DIM>*)node);
}

template <unsigned int DIM>
void PrefixSharingVisitor<DIM>::visitGeneral(const Node<DIM>* node) {
	prefixSharedBits += node->prefix_.getBitLength() * node->prefix_.getDim();
	prefixBitsWithoutSharing += DIM * node->prefix_.getBitLength() * node->getNumberOfContents();
}

template <unsigned int DIM>
void PrefixSharingVisitor<DIM>::reset() {
	prefixBitsWithoutSharing = 0;
	prefixSharedBits = 0;
}

template <unsigned int DIM>
unsigned long PrefixSharingVisitor<DIM>::getPrefixSharedBits() const {
	return prefixSharedBits;
}

template <unsigned int DIM>
unsigned long PrefixSharingVisitor<DIM>::getPrefixBitsWithoutSharing() const {
	return prefixBitsWithoutSharing;
}

template <unsigned int D>
std::ostream& operator <<(std::ostream &out, const PrefixSharingVisitor<D>& v) {
	return v.output(out);
}

template <unsigned int DIM>
std::ostream& PrefixSharingVisitor<DIM>::output(std::ostream &out) const {
	float sharingSavedPercent = float(getPrefixSharedBits()) / float(getPrefixBitsWithoutSharing());
	return out << "total shared prefixes: " << getPrefixSharedBits() << " bits"
			<< " (only " << sharingSavedPercent << "% of otherwise "
			<< getPrefixBitsWithoutSharing() << " bits)" << endl;
}

#endif /* SRC_VISITORS_PREFIXSHARINGVISITOR_H_ */
