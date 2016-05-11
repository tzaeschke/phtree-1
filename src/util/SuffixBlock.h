/*
 * SuffixBlock.h
 *
 *  Created on: May 10, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_SUFFIXBLOCK_H_
#define SRC_UTIL_SUFFIXBLOCK_H_

template <unsigned int DIM>
class SizeVisitor;

template <unsigned int SUFF_PER_BLOCK>
class SuffixBlock {
	template <unsigned int D>
	friend class SizeVisitor;
public:
	SuffixBlock();
	virtual ~SuffixBlock();
	unsigned long* reserveSuffixBlocks(unsigned int nBlocks, SuffixBlock<SUFF_PER_BLOCK>** endBlock);

private:
	unsigned int currentBlock;
	SuffixBlock<SUFF_PER_BLOCK>* next;
	unsigned long suffixBlocks[SUFF_PER_BLOCK];
};

using namespace std;
#include <assert.h>

template <unsigned int SUFF_PER_BLOCK>
SuffixBlock<SUFF_PER_BLOCK>::SuffixBlock() : currentBlock(0), next(nullptr), suffixBlocks() { }

template <unsigned int SUFF_PER_BLOCK>
SuffixBlock<SUFF_PER_BLOCK>::~SuffixBlock() {
	delete next;
}

template <unsigned int SUFF_PER_BLOCK>
unsigned long* SuffixBlock<SUFF_PER_BLOCK>::reserveSuffixBlocks(unsigned int nBlocks,
		SuffixBlock<SUFF_PER_BLOCK>** endBlock) {

	assert (nBlocks <= SUFF_PER_BLOCK);
	SuffixBlock<SUFF_PER_BLOCK>* freeBlock = this;
	// find first block to reserve n consecutive blocks from
	while (nBlocks > SUFF_PER_BLOCK - freeBlock->currentBlock) {
		if (!freeBlock->next) {
			freeBlock->next = new SuffixBlock<SUFF_PER_BLOCK>();
			(*endBlock) = freeBlock->next;
		}

		freeBlock = freeBlock->next;
	}

	unsigned long* startBlock = freeBlock->suffixBlocks + freeBlock->currentBlock;
	freeBlock->currentBlock += nBlocks;
	return startBlock;
}

#endif /* SRC_UTIL_SUFFIXBLOCK_H_ */
