/*
 * AssertionVisitor.h
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#ifndef VISITORS_ASSERTIONVISITOR_H_
#define VISITORS_ASSERTIONVISITOR_H_

#include "visitors/Visitor.h"

template <unsigned int DIM>
class NodeIterator;

template <unsigned int DIM>
class Node;

template <unsigned int DIM>
class AssertionVisitor: public Visitor<DIM> {
public:
	AssertionVisitor();
	virtual ~AssertionVisitor();

	virtual void visit(LHC<DIM>* node, unsigned int depth) override;
	virtual void visit(AHC<DIM>* node, unsigned int depth) override;
	virtual void reset() override;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	void validateContents(const Node<DIM>* node, NodeIterator<DIM>* begin, NodeIterator<DIM>* end);
};

/* template implemenation */
#include <assert.h>
#include "iterators/NodeIterator.h"
#include "nodes/Node.h"
#include "nodes/LHC.h"
#include "nodes/AHC.h"

template <unsigned int DIM>
AssertionVisitor<DIM>::AssertionVisitor() : Visitor<DIM>() {
}

template <unsigned int DIM>
AssertionVisitor<DIM>::~AssertionVisitor() {
}

template <unsigned int DIM>
void AssertionVisitor<DIM>::visit(LHC<DIM>* node, unsigned int depth) {
	validateContents(node, node->begin(), node->end());
}

template <unsigned int DIM>
void AssertionVisitor<DIM>::visit(AHC<DIM>* node, unsigned int depth) {
	validateContents(node, node->begin(), node->end());
}

template <unsigned int DIM>
void AssertionVisitor<DIM>::validateContents(const Node<DIM>* node, NodeIterator<DIM>* begin, NodeIterator<DIM>* end) {
	bool foundSuffix = false;
		size_t suffixLength = 0;
		for (NodeIterator<DIM>* it = begin; (*it) != *(end); ++(*it)) {
			NodeAddressContent<DIM> content = *(*it);
			if (!foundSuffix && content.exists && !content.hasSubnode) {
				suffixLength = content.suffix->size();
				foundSuffix = true;
			}

			assert(content.exists);
			assert((content.hasSubnode || content.suffix->size() == suffixLength)
							&& "all suffixes in one node should have the same length");
		}
		delete begin;
		delete end;
}

template <unsigned int DIM>
void AssertionVisitor<DIM>::reset() {
}

template <unsigned int DIM>
std::ostream& AssertionVisitor<DIM>::output(std::ostream &out) const {
	return out;
}

#endif /* VISITORS_ASSERTIONVISITOR_H_ */
