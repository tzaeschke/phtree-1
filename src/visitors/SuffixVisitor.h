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

	unsigned long externallyStoredSuffixBits;
	unsigned long externalSuffixBlocks;
	unsigned long internallyStoredSuffixBits;
	unsigned long internallyStoredSuffixes;
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

	const unsigned long currentSuffixBits = DIM * (treeWidth - 1 - index - 1);
	assert (currentSuffixBits <= DIM * (treeWidth - 1));

	NodeIterator<DIM>* it;
	NodeIterator<DIM>* endIt = node->end();
	for (it = node->begin(); (*it) != *endIt; ++(*it)) {
		NodeAddressContent<DIM> content = *(*it);
		if (!content.hasSubnode) {
			visitedSuffixes++;
			if (content.directlyStoredSuffix) {
				++internallyStoredSuffixes;
				assert (currentSuffixBits <= sizeof (uintptr_t) * 8 - 2);
				internallyStoredSuffixBits += currentSuffixBits;
			} else {
				externallyStoredSuffixBits += currentSuffixBits;
				externalSuffixBlocks += 1uL + currentSuffixBits / MultiDimBitset<DIM>::bitsPerBlock;
			}
		}
	}

	delete it;
	delete endIt;
}

template <unsigned int DIM>
void SuffixVisitor<DIM>::reset() {
	visitedSuffixes = 0;
	internallyStoredSuffixes = 0;
	treeWidth = 0;
	externallyStoredSuffixBits = 0;
	externalSuffixBlocks = 0;
	internallyStoredSuffixBits = 0;
}

template <unsigned int D>
std::ostream& operator <<(std::ostream &out, const SuffixVisitor<D>& v) {
	return v.output(out);
}

template <unsigned int DIM>
std::ostream& SuffixVisitor<DIM>::output(std::ostream &out) const {

	const size_t externallyStoredSuffixes = visitedSuffixes - internallyStoredSuffixes;
	const double internallyStoredRatioPercent = double(internallyStoredSuffixes) / double(visitedSuffixes) * 100.0;
	const double avgInternalSuffixBits = double(internallyStoredSuffixBits) / double(internallyStoredSuffixes);
	const double avgExternalSuffixBits = double(externallyStoredSuffixBits) / double(externallyStoredSuffixes);
	const double avgExternalSuffixBlocks = double(externalSuffixBlocks) / double(externallyStoredSuffixes);

	return out << "suffixes internally stored: " << internallyStoredSuffixes << " / "
			<< internallyStoredRatioPercent << "% (avg "
			<< avgInternalSuffixBits << " bits), avg external bits: " << avgExternalSuffixBits << " bits ("
			<< avgExternalSuffixBlocks << " blocks)" << endl;
}


#endif /* SRC_VISITORS_SUFFIXVISITOR_H_ */
