/*
 * MultiDimBitset.h
 *
 *  Created on: Apr 18, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_MULTIDIMBITSET_H_
#define SRC_UTIL_MULTIDIMBITSET_H_

#include <vector>
#include <iostream>


template <unsigned int DIM>
class MultiDimBitset {
public:
	static const size_t bitsPerBlock = sizeof(unsigned long) * 8;
	static const size_t filledBlock = -1; // a block with all bits set to 1

	template <unsigned int WIDTH>
	static void toBitset(std::vector<unsigned long> &values, unsigned long* outStartBlock);
	static std::pair<bool, size_t> compare(const unsigned long* startBlock, unsigned int nBits,
			size_t fromIndex, size_t toIndex, const unsigned long* otherStartBlock, unsigned int otherNBits);
	static std::vector<unsigned long> toLongs(const unsigned long* fromStartBlock, size_t nBits);
	static unsigned long interleaveBits(const unsigned long* fromStartBlock, size_t index, size_t nBits);
	// TODO add: remove x <= 64 bits within block method by applying a bit mask
	static void removeHighestBits(const unsigned long* startBlock, unsigned int nBits, size_t nBitsToRemove, unsigned long* outStartBlock);
	static void duplicateHighestBits(const unsigned long* startBlock, unsigned int nBits, unsigned int nBitsToDuplicate, unsigned long* outStartBlock);
	static size_t calculateLongestCommonPrefix(const unsigned long* startBlock, unsigned int nBits, size_t startIndex,
			const unsigned long* otherStartBlock, unsigned int otherNBits, unsigned long* outStartBlock);
	static std::ostream& output(std::ostream &os, const unsigned long* startBlock, unsigned int nBits);

private:
	static inline std::pair<bool, size_t> compareAlignedBlocks(const unsigned long b1, const unsigned long b2);
};

#include <string.h>
#include <float.h>
#include <math.h>

using namespace std;

template <unsigned int DIM>
template <unsigned int WIDTH>
void MultiDimBitset<DIM>::toBitset(std::vector<unsigned long> &values, unsigned long* outStartBlock) {
	//     example 2 Dim, 8 Bit: (    10   ,     5    )
	//    binary representation: (0000 1010, 0000 0101)
	//  				  index:    12    8    4    0
	// after conversion (mixed): (0000 0000 0110 0110) = 102

	assert (sizeof (unsigned long) * 8 >= WIDTH);
	assert (values.size() == DIM);
	assert (outStartBlock[0] == 0);
	const size_t nBits = DIM * WIDTH;
	// TODO change loop order?
	for (size_t d = 0; d < DIM; ++d) {
		for (size_t i = 0; i < WIDTH; ++i) {
			// extract j-th least segnificant bit from int
			// TODO use CPU instruction for interleaving
			const unsigned long bit = (values[d] >> i) & 1;
			const auto block = (DIM * i + d) / bitsPerBlock;
			const auto index = (DIM * i + d) % bitsPerBlock;
			*(outStartBlock + block) |= bit << index;
		}
	}

	assert(toLongs(outStartBlock, nBits) == values);
}


template <unsigned int DIM>
vector<unsigned long> MultiDimBitset<DIM>::toLongs(const unsigned long* fromStartBlock, size_t nBits) {

		vector<unsigned long> numericalValues(DIM, 0);
		const size_t bitLength = nBits / DIM;
		// TODO change loop order?
		for (size_t i = 0; i < bitLength; ++i) {
			for (size_t d = 0; d < DIM; ++d) {
				const size_t blockIndex = (DIM * i + d) / bitsPerBlock;
				const size_t bitIndex = (DIM * i + d) % bitsPerBlock;
				const size_t block = fromStartBlock[blockIndex];
				const size_t bit = (block >> bitIndex) & 1;
				numericalValues[d] += bit << i;
			}
		}

		return numericalValues;
}

template <unsigned int DIM>
std::ostream& MultiDimBitset<DIM>::output(std::ostream &os, const unsigned long* startBlock, unsigned int nBits) {
	// bits are stored in ascending order so they need to be printed in reverse oder
	assert (nBits % DIM == 0);
	const size_t bitLength = nBits / DIM;
	os << "(";
	for (size_t d = 0; d < DIM; ++d) {
		os << "(";
		for (long i = bitLength - 1; i >= 0; --i) {
			const auto blockIndex = (DIM * i + d) / bitsPerBlock;
			const auto bitIndex = (DIM * i + d) % bitsPerBlock;
			const auto block = *(startBlock + blockIndex);
			const auto bit = ((block & (1uL << bitIndex)) >> bitIndex);
			os << bit;
		}
		os << ")";
		if (d < DIM - 1) {
			os << ", ";
		}
	}

	os << ")";
	return os;
}

template <unsigned int DIM>

std::pair<bool, size_t> MultiDimBitset<DIM>::compare(const unsigned long* startBlock, unsigned int nBits,
		size_t fromMsbIndex, size_t toMsbIndex, const unsigned long* otherStartBlock, unsigned int otherNBits) {
	assert (DIM > 0);

	const size_t upper = nBits - DIM * fromMsbIndex;
	const size_t lower = nBits - DIM * toMsbIndex;
	const size_t lsbRange = upper - lower;
	const size_t b_max = bitsPerBlock;
	assert(lower <= upper);

	if (upper == lower) {
		return pair<bool, size_t>(true, 0);
	}

	assert(nBits >= upper);
	assert(lsbRange == otherNBits);
	assert(lsbRange % DIM == 0);

	const size_t x = upper % b_max;
	const size_t y = lower % b_max;
	const long b_high = upper / b_max;
	const long b_low = lower / b_max;
	// extract lower part of a block [  **y**  |  b_max - y  ]
	const unsigned long lowerBlockMask = (1uL << y) - 1uL;
	// extract upper part of a block [  y  |  **b_max - y**  ]
	const unsigned long upperBlockMask = filledBlock << y;
	// removes bits after upper index from highest block: [  x  | 0 ... 0]
	const unsigned long highestBlockMask = (1uL << x) - 1uL;
	assert ((lowerBlockMask ^ upperBlockMask) == filledBlock);
	const size_t highestFreeBits = b_max - x;

	long longestCommonPrefix = 0;
	bool allDimSame = true;
	unsigned long currentAlignedBlock = 0uL;
	const unsigned long highestBlock = startBlock[b_high] & highestBlockMask;
	// handle the highest block depending on the given offsets
	if (x > y) {
		// the x range overlapps with the necessary slot so split it
		// from: [        x        |     ]
		// into: [  y  |  rest     | free]
		// compare [rest | free] to highest block and remove the rest
		const unsigned long highestAlignedBlock = highestBlock >> y;
		const size_t otherHighestBlockIndex = (otherNBits - 1) / bitsPerBlock;
		const unsigned long otherHighestBlock = otherStartBlock[otherHighestBlockIndex];
		pair<bool, size_t> comp = compareAlignedBlocks(highestAlignedBlock, otherHighestBlock);
		allDimSame = comp.first;
		assert (comp.second >= y + highestFreeBits);
		longestCommonPrefix = comp.second - y - highestFreeBits;
		// [  y  |    free   ]
		currentAlignedBlock = highestBlock & lowerBlockMask;
	} else {
		// attention: this cannot happen if the range is within a single block
		// will compare the x bits of this block in the following for-loop
		currentAlignedBlock = highestBlock;
		// will later be set to: [ b_max - y  |  x  |  free]
		// set the prefix length to the number of trailing free bits
		longestCommonPrefix = -(y - x);
	}

	for (long b = b_high - 1; b >= b_low && allDimSame; --b) {
		// [  y(t)  |  b_max - y  (t)]
		unsigned long thisBlock = startBlock[b];
		// [  y(t)  |  0    ...     0]
		const unsigned long lowerThisBlockPart = thisBlock & lowerBlockMask;
		// [0 ... 0 |  b_max - y  (t)]
		const unsigned long upperThisBlockPart = thisBlock & upperBlockMask;
		// [ 0    ...    0 |  y(t+1) ]
		currentAlignedBlock <<= b_max - y;
		// [ b_max - y  (t)|  y(t+1) ]
		currentAlignedBlock |= (upperThisBlockPart >> y);

		assert (0 <= b - b_low && otherNBits > b - b_low);
		const unsigned long otherBlock = otherStartBlock[b - b_low];
		pair<bool, size_t> comp = compareAlignedBlocks(currentAlignedBlock, otherBlock);
		allDimSame = comp.first;
		longestCommonPrefix += comp.second;

		// [  y(t)  |  0    ...     0]
		currentAlignedBlock = lowerThisBlockPart;
	}

	longestCommonPrefix /= DIM;
	assert (longestCommonPrefix >= 0 && longestCommonPrefix <= lsbRange / DIM);
	assert (!allDimSame || otherNBits / DIM == longestCommonPrefix);
	return pair<bool, size_t>(allDimSame, longestCommonPrefix);
}

template <unsigned int DIM>
pair<bool, size_t> MultiDimBitset<DIM>::compareAlignedBlocks(const unsigned long b1, const unsigned long b2) {
	unsigned long comparison = b1 ^ b2;
	size_t longestCommonPrefix = sizeof (unsigned long) * 8;
	bool allDimSame = comparison == 0;
	if (!allDimSame) {
		longestCommonPrefix = __builtin_clzl(comparison);
	}

	return pair<bool, size_t>(allDimSame, longestCommonPrefix);
}

template <unsigned int DIM>
unsigned long MultiDimBitset<DIM>::interleaveBits(const unsigned long* fromStartBlock, size_t msbIndex, size_t nBits) {
		assert (nBits >= DIM * (msbIndex + 1));

		// TODO use blockwise operation
		unsigned long hcAddress;
		// the index addresses bits in descending order but internally bits are stored ascendingly
		const size_t lsbStartIndex = nBits - DIM * (msbIndex + 1);
		const size_t lsbEndIndex = lsbStartIndex + DIM;
		const size_t b_max = bitsPerBlock;
		const size_t startBlock = lsbStartIndex / b_max;
		const size_t endBlock = lsbEndIndex / b_max;
		assert (endBlock - startBlock <= 1);

		const unsigned long block = fromStartBlock[startBlock];
		const size_t lower = lsbStartIndex % b_max;

		if (startBlock == endBlock || lsbEndIndex % b_max == 0) {
			const unsigned long maskForInterleaved = (1uL << DIM) - 1uL;
			hcAddress = (block >> lower) & maskForInterleaved;
		} else {
			const size_t upper = lsbEndIndex % b_max;
			const unsigned long maskForStartBlock = filledBlock << lower;
			const unsigned long maskForEndBlock = (1uL << upper) - 1;
			const unsigned long secondBlock = fromStartBlock[endBlock];
			const unsigned long shifSecond = (startBlock + 1) * b_max - lsbStartIndex;
			assert (shifSecond < DIM && shifSecond > 0);
			hcAddress = ((block & maskForStartBlock) >> lower) | ((secondBlock & maskForEndBlock) << shifSecond);
		}

		assert (hcAddress < (1ul<<DIM));
		return hcAddress;
}



template <unsigned int DIM>
void MultiDimBitset<DIM>::removeHighestBits(const unsigned long* startBlock,
		unsigned int nBits, size_t nBitsToRemove, unsigned long* outStartBlock) {
	assert (outStartBlock);
	assert (nBits >= nBitsToRemove * DIM);

	const auto initialSize = nBits;
	if (nBitsToRemove * DIM == initialSize) {
		// no need to remove all bits
		return;
	}

	// copy all blocks before the block to be cut
	const auto remainingBits = initialSize - nBitsToRemove * DIM;
	const auto cutBlockIndex = remainingBits / bitsPerBlock;
	for (size_t b = 0; b < cutBlockIndex; ++b) {
		const unsigned long block = startBlock[b];
		outStartBlock[b] = block;
	}

	// copy the block in which the cut occurred and cut highest bits
	const auto cutBitIndex = remainingBits % bitsPerBlock;
	const auto lowestBitsMask = filledBlock >> (bitsPerBlock - cutBitIndex);
	const unsigned long block = startBlock[cutBlockIndex] & lowestBitsMask;
	outStartBlock[cutBlockIndex] = block;

	// removed further bits by simply not copying them
}

/*template <unsigned int DIM>
void MultiDimBitset<DIM>::pushBitsToBack(const MultiDimBitset<DIM>* source) {
	assert(source);
	assert(source->getDim() == DIM);
	assert (bits.bits_per_block == source->bits.bits_per_block);

	if (source->bits.empty()) {
		return;
	}

	// before push: | before   \ free |
	// source:      | v_1 \   r_1     | , | v_2 \   r_2     | , | v_3 \ r_3 \ free|
	//              | vL  \  rL       | , |-----\-----------| , |-----\-----------|
	// after push:  | before   \ v_1  | , |   r_1    \ v_2  | , |   r_2     \ v_3 | , |   r_3  \ free  |

	const size_t initialSize = bits.size();
	const unsigned long vL = initialSize % bits.bits_per_block;
	const unsigned long rL = bits.bits_per_block - vL;
	const unsigned long vMask = (1uL << vL) - 1;
	const unsigned long rMask = ULLONG_MAX << vL;
	assert (vL + rL == bits.bits_per_block && ((vMask ^ rMask) == ULLONG_MAX));

	const auto lastBlockIndex = initialSize / bits.bits_per_block;
	const auto sourceBlocks = source->bits.size() / source->bits.bits_per_block;

	// set value in last unfilled block
	unsigned long sourceBlock = source->bits.m_bits[0];
	unsigned long v = sourceBlock & vMask;
	unsigned long r = sourceBlock & rMask;
	bits.m_bits[lastBlockIndex] &= v << rL;
	unsigned long currentBlock = r;

	// add current value to free space in current block and set remainder to next block
	for (size_t b = 1; b < sourceBlocks; ++b) {
		const unsigned long sourceBlock = source->bits.m_bits[b];
		const unsigned long v = sourceBlock & vMask;
		const unsigned long r = sourceBlock & rMask;
		currentBlock &= v << rL;
		bits.append(currentBlock);
		currentBlock = r;
	}
	bits.append(currentBlock);
	bits.resize(initialSize + source->bits.size());

	assert(this->size() == initialSize + source->size());
}

template <unsigned int DIM>
void MultiDimBitset<DIM>::pushValueToBack(unsigned long newValue) {
		boost::dynamic_bitset<> convertedValue = longToBitset(newValue, DIM);
		assert (convertedValue.size() == DIM);
		const size_t initialSize = bits.size();

		bits.resize(initialSize + DIM);
		for (size_t i = 0; i < DIM; ++i) {
			bits[i] = convertedValue[i];
		}

		assert (bits.size() == initialSize + DIM);
}*/

template <unsigned int DIM>
void MultiDimBitset<DIM>::duplicateHighestBits(const unsigned long* startBlock,
		unsigned int nBits, unsigned int nBitsToDuplicate, unsigned long* outStartBlock) {
	assert (outStartBlock);
	assert (nBits >= DIM * nBitsToDuplicate);

	if (nBitsToDuplicate == 0) {
		return;
	}

	// duplicate:       <------------------------------------------------>
	// this:      | v_1 \   r_1     | , | v_2 \   r_2     | , | v_3 \ r_3 \ free|
	//            | vL  \    rL     | , |-----\-----------| , |-----\-----------|
	// result:    |   r_1    \ v_2  | , |   r_2     \ v_3 | , |  r_3 \    free  |

	const size_t initialSize = nBits;
	const size_t neglectedLSBBits = initialSize - DIM * nBitsToDuplicate;
	const size_t startCopyBlock = neglectedLSBBits / bitsPerBlock;
	const size_t lastBlockIndex = initialSize / bitsPerBlock;
	const unsigned long vL = neglectedLSBBits % bitsPerBlock;
	const unsigned long rL = bitsPerBlock - vL;
	const unsigned long vMask = (1uL << vL) - 1;
	const unsigned long rMask = filledBlock << vL;
	assert ((vL + rL == bitsPerBlock) && ((vMask ^ rMask) == filledBlock));

	unsigned long sourceBlock = startBlock[startCopyBlock];
	unsigned long r = (sourceBlock & rMask) >> vL;
	unsigned long currentBlock = r;
	// add current value to free space in current block and set remainder to next block
	for (size_t b = startCopyBlock + 1; b < lastBlockIndex + 1; ++b) {
		const unsigned long sourceBlock = startBlock[b];
		const unsigned long v = (sourceBlock & vMask) << rL;
		const unsigned long r = (sourceBlock & rMask) >> vL;
		currentBlock |= v;
		outStartBlock[b - (startCopyBlock + 1)] = currentBlock;
		currentBlock = r;
	}
	outStartBlock[lastBlockIndex - startCopyBlock] = currentBlock;
	// TODO might need to set the nBits of the out blocks or remove the trailing ones using a mask
}

// find the longest common prefix starting at the msbStartIndex and sets it to the result to reference
template <unsigned int DIM>
size_t MultiDimBitset<DIM>::calculateLongestCommonPrefix(const unsigned long* startBlock,
		unsigned int nBits, size_t msbStartIndex, const unsigned long* otherStartBlock,
		unsigned int otherNBits, unsigned long* outStartBlock) {

	assert (startBlock && otherStartBlock && outStartBlock);
	assert (nBits - DIM * msbStartIndex == otherNBits);

	const size_t otherBitLength = otherNBits / DIM;
	const size_t msbEndIndex = msbStartIndex + otherBitLength;
	pair<bool, unsigned long> comparison = compare(startBlock, nBits, msbStartIndex,
			msbEndIndex, otherStartBlock, otherNBits);
	const size_t prefixLength = comparison.second;
	assert (prefixLength <= otherBitLength);
	duplicateHighestBits(otherStartBlock, otherNBits, prefixLength, outStartBlock);
	return prefixLength;
}

#endif /* SRC_UTIL_MULTIDIMBITSET_H_ */
