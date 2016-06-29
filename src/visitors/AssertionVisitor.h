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

	template <unsigned int WIDTH>
	void visitSub(PHTree<DIM, WIDTH>* tree);
	template <unsigned int PREF_BLOCKS, unsigned int N>
	void visitSub(LHC<DIM, PREF_BLOCKS, N>* node, unsigned int depth);
	template <unsigned int PREF_BLOCKS>
	void visitSub(AHC<DIM, PREF_BLOCKS>* node, unsigned int depth);
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
template <unsigned int WIDTH>
void AssertionVisitor<DIM>::visitSub(PHTree<DIM, WIDTH>* tree) {

}

template <unsigned int DIM>
template <unsigned int PREF_BLOCKS, unsigned int N>
void AssertionVisitor<DIM>::visitSub(LHC<DIM, PREF_BLOCKS, N>* node, unsigned int depth) {
	assert (node->getNumberOfContents() > 0);

	unsigned long lastHcAddress = -1;
	bool hasSubnode = false;
	bool directlyStored = false;
	NodeAddressContent<DIM> content;
	node->lookupIndex(0, &lastHcAddress, &hasSubnode, &directlyStored);
	for (unsigned int i = 1; i < node->m; ++i) {
		unsigned long hcAddress = 0;
		node->lookupIndex(i, &hcAddress, &hasSubnode, &directlyStored);
		assert (hcAddress > lastHcAddress);
		bool exists = false;
		bool hasSubnodeTest = false;
		bool directlyStoredTest = false;
		unsigned int indexTest = -1;
		node->lookupAddress(hcAddress, &exists, &indexTest, &hasSubnodeTest, &directlyStoredTest);
		assert (indexTest == i);
		assert (exists);
		assert (hasSubnode == hasSubnodeTest && directlyStored == directlyStoredTest);
		node->lookup(hcAddress, content, true);
		assert (content.exists && content.address == hcAddress && content.hasSubnode == hasSubnode);
		lastHcAddress = hcAddress;
	}
}

template <unsigned int DIM>
template <unsigned int PREF_BLOCKS>
void AssertionVisitor<DIM>::visitSub(AHC<DIM, PREF_BLOCKS>* node, unsigned int depth) {
	validateContents(node, node->begin(), node->end());
}

template <unsigned int DIM>
void AssertionVisitor<DIM>::validateContents(const Node<DIM>* node, NodeIterator<DIM>* begin, NodeIterator<DIM>* end) {
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
