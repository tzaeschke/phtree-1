/*
 * SuffixVisitor.h
 *
 *  Created on: May 19, 2016
 *      Author: max
 */

#ifndef SRC_VISITORS_SUFFIXVISITOR_H_
#define SRC_VISITORS_SUFFIXVISITOR_H_


#include "visitors/Visitor.h"

template <unsigned int DIM, unsigned int PREF_BLOCKS>
class TNode;

template <unsigned int DIM>
class SuffixVisitor: public Visitor<DIM> {
	template <unsigned int D>
	friend std::ostream& operator <<(std::ostream &out, const SuffixVisitor<D>& v);
public:
	SuffixVisitor();
	virtual ~SuffixVisitor();

	template <unsigned int WIDTH>
	void visitSub(PHTree<DIM, WIDTH>* tree);
	template <unsigned int PREF_BLOCKS, unsigned int N>
	void visitSub(LHC<DIM, PREF_BLOCKS, N>* node, unsigned int depth, unsigned int index);
	template <unsigned int PREF_BLOCKS>
	void visitSub(AHC<DIM, PREF_BLOCKS>* node, unsigned int depth, unsigned int index);
	virtual void reset() override;

	unsigned long getPrefixSharedBits() const;
	unsigned long getPrefixBitsWithoutSharing() const;

protected:
	std::ostream& output(std::ostream &out) const override;

private:
	template <unsigned int PREF_BLOCKS>
	void visitGeneral(const TNode<DIM, PREF_BLOCKS>* node, unsigned int index);

	unsigned long suffixBits;
	unsigned long suffixBlocks;
	unsigned long visitedSuffixes;
	unsigned long treeWidth;
};

#include "nodes/TNode.h"
#include "iterators/NodeIterator.h"

template <unsigned int DIM>
SuffixVisitor<DIM>::SuffixVisitor() : Visitor<DIM>() {
	reset();
}

template <unsigned int DIM>
SuffixVisitor<DIM>::~SuffixVisitor() {
}

template <unsigned int DIM>
template <unsigned int WIDTH>
void SuffixVisitor<DIM>::visitSub(PHTree<DIM, WIDTH>* tree) {
	treeWidth = WIDTH;
}

template <unsigned int DIM>
template <unsigned int PREF_BLOCKS, unsigned int N>
void SuffixVisitor<DIM>::visitSub(LHC<DIM, PREF_BLOCKS, N>* node, unsigned int depth, unsigned int index) {
	this->template visitGeneral<PREF_BLOCKS>(node, index);
}

template <unsigned int DIM>
template <unsigned int PREF_BLOCKS>
void SuffixVisitor<DIM>::visitSub(AHC<DIM, PREF_BLOCKS>* node, unsigned int depth, unsigned int index) {
	this->template visitGeneral<PREF_BLOCKS>(node, index);
}

template <unsigned int DIM>
template <unsigned int PREF_BLOCKS>
void SuffixVisitor<DIM>::visitGeneral(const TNode<DIM, PREF_BLOCKS>* node, unsigned int index) {

	const unsigned long currentSuffixBits = DIM * (treeWidth - index - 1);

	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = node->end();
	for (it = node->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		if (!content.hasSubnode) {
			visitedSuffixes++;
			suffixBits += currentSuffixBits;
			suffixBlocks += 1 + currentSuffixBits / MultiDimBitset<DIM>::bitsPerBlock;
		}
	}

	delete it;
	delete endIt;
}

template <unsigned int DIM>
void SuffixVisitor<DIM>::reset() {
	suffixBits = 0;
	suffixBlocks = 0;
	visitedSuffixes = 0;
	treeWidth = 0;
}

template <unsigned int D>
std::ostream& operator <<(std::ostream &out, const SuffixVisitor<D>& v) {
	return v.output(out);
}

template <unsigned int DIM>
std::ostream& SuffixVisitor<DIM>::output(std::ostream &out) const {

	const size_t avgSuffixBits = suffixBits / visitedSuffixes;
	const size_t avgSuffixBlocks = suffixBlocks / visitedSuffixes;
	return out << "average per suffix: "
			<< avgSuffixBits << " bits, "
			<< avgSuffixBlocks << " block(s)"
			<< " (max " << ((treeWidth - 1) * DIM) << " suffix bits)"<< endl;
}


#endif /* SRC_VISITORS_SUFFIXVISITOR_H_ */
