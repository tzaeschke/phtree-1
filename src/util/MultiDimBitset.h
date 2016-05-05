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

	static void toBitset(std::vector<unsigned long> &values, unsigned long* outStartBlock);
	static std::pair<bool, size_t> compare(const unsigned long* startBlock, size_t fromIndex, size_t toIndex, const unsigned long* otherStartBlock);
	static std::vector<unsigned long> toLongs(const unsigned long* fromStartBlock, size_t nBits);
	static unsigned long interleaveBits(const unsigned long* fromStartBlock, size_t index);
	static void removeHighestBits(const unsigned long* startBlock, unsigned int nBits, size_st nBitsToRemove, unsigned long* outStartBlock);
	static void duplicateHighestBits(const unsigned long* startBlock, unsigned int nBits. unsigned int nBitsToDuplicate, unsigned long* outStartBlock);
	static size_t calculateLongestCommonPrefix(const unsigned long* startBlock, unsigned int nBits, size_t startIndex,
			const unsigned long* otherStartBlock, const unsigned long* outStartBlock);

private:
	static boost::dynamic_bitset<> longToBitset(unsigned long value, size_t bitLength);
	static inline std::pair<bool, size_t> compareAlignedBlocks(const unsigned long b1, const unsigned long b2);
};

#include <string.h>
#include <float.h>
#include <math.h>

using namespace std;


template <unsigned int DIM>
void MultiDimBitset<DIM>::toBitset(std::vector<unsigned long> &values, unsigned long* outStartBlock) {
	// example 2 Dim, 8 Bit:     (   10    ,    5     )
	// binary representation:    (0000 1010, 0000 0101)
	// index:                       12    8    4    0
	// after conversion (mixed): (0000 0000 0110 0110)

	assert (values.size() == DIM);
	const size_t nBits = values.size() * sizeof(unsigned long);
		// TODO rewrite to directly convert the values
		for (size_t d = 0; d < DIM; ++d) {
			boost::dynamic_bitset<> entry = longToBitset(values[d], bitLength);
			for (size_t i = 0; i < bitLength; ++i) {
				bits[DIM * i + d] = entry[i];
			}
		}

	assert (this->size() == DIM * bitLength);

}

template <unsigned int DIM>
boost::dynamic_bitset<> MultiDimBitset<DIM>::longToBitset(unsigned long value, size_t bitLength) {
	boost::dynamic_bitset<> convertedValue(bitLength);
	for (size_t i = 0; i < bitLength; i++) {
		// extract j-th least segnificant bit from int
		// TODO make use of bitset functions
		bool bit = ((value & (1 << i)) >> i) == 1;
		convertedValue[i] = bit;
	}
	return convertedValue;
}

template <unsigned int DIM>
MultiDimBitset<DIM>::~MultiDimBitset() {
	bits.clear();
}

template <unsigned int DIM>
bool MultiDimBitset<DIM>::operator ==(const MultiDimBitset<DIM> &b) const {
	return this->bits == b.bits;
}

template <unsigned int DIM>
bool MultiDimBitset<DIM>::operator !=(const MultiDimBitset<DIM> &b) const {
	return this->bits != b.bits;
}

template <unsigned int D>
std::ostream& operator <<(std::ostream &os, const MultiDimBitset<D> &bitset) {
	// bits are stored in ascending order so they need to be printed in reverse oder
	const size_t bitLength = bitset.getBitLength();
	for (size_t d = 0; d < D; ++d) {
		os << "(";
		for (long i = bitLength - 1; i >= 0; --i) {
			const int bitNumber = (bitset.bits[i * D + d]) ? 1 : 0;
			os << bitNumber;
		}
		os << ")";
		if (d < D - 1) {
			os << ", ";
		}
	}

	os << ")";
	return os;
}

template <unsigned int DIM>
size_t MultiDimBitset<DIM>::size() const {
	return bits.size();
}

template <unsigned int DIM>
size_t MultiDimBitset<DIM>::getBitLength() const {
	if (DIM == 0) {
		return 0;
	}

	return bits.size() / DIM;
}

template <unsigned int DIM>
inline size_t MultiDimBitset<DIM>::inlineBitLength() const {
	return bits.size() / DIM;
}

template <unsigned int DIM>
size_t MultiDimBitset<DIM>::getDim() const {
	return DIM;
}

template <unsigned int DIM>
void MultiDimBitset<DIM>::clear() {
	bits.clear();
}

template <unsigned int DIM>
pair<bool, size_t> MultiDimBitset<DIM>::compareTo(size_t fromMsbIndex, size_t toMsbIndex,
		const MultiDimBitset<DIM> &other) const {
	assert (DIM > 0);

	const size_t upper = DIM * (inlineBitLength() - fromMsbIndex);
	const size_t lower = DIM * (inlineBitLength() - toMsbIndex);
	const size_t lsbRange = upper - lower;
	const size_t b_max = bits.bits_per_block;
	assert(lower <= upper);

	if (other.bits.empty() || upper == lower) {
		return pair<bool, size_t>(true, 0);
	}

	assert(this->getDim() == other.getDim());
	assert(this->size() >= upper);
	assert(lsbRange == other.bits.size());
	assert(lsbRange % DIM == 0);

	const size_t x = upper % b_max;
	const size_t y = lower % b_max;
	const long b_high = upper / b_max;
	const long b_low = lower / b_max;
	// extract lower part of a block [  **y**  |  b_max - y  ]
	const unsigned long lowerBlockMask = (1uL << y) - 1uL;
	// extract upper part of a block [  y  |  **b_max - y**  ]
	const unsigned long upperBlockMask = ULLONG_MAX << y;
	// removes bits after upper index from highest block: [  x  | 0 ... 0]
	const unsigned long highestBlockMask = (1uL << x) - 1uL;
	assert ((lowerBlockMask ^ upperBlockMask) == ULLONG_MAX);
	const size_t highestFreeBits = b_max - x;

	long longestCommonPrefix = 0;
	bool allDimSame = true;
	unsigned long currentAlignedBlock = 0uL;
	const unsigned long highestBlock = bits.m_bits[b_high] & highestBlockMask;
	// handle the highest block depending on the given offsets
	if (x > y) {
		// the x range overlapps with the necessary slot so split it
		// from: [        x        |     ]
		// into: [  y  |  rest     | free]
		// compare [rest | free] to highest block and remove the rest
		const unsigned long highestAlignedBlock = highestBlock >> y;
		const unsigned long otherHighestBlock = other.bits.m_bits[other.bits.num_blocks() - 1];
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
		unsigned long thisBlock = this->bits.m_bits[b];
		// [  y(t)  |  0    ...     0]
		const unsigned long lowerThisBlockPart = thisBlock & lowerBlockMask;
		// [0 ... 0 |  b_max - y  (t)]
		const unsigned long upperThisBlockPart = thisBlock & upperBlockMask;
		// [ 0    ...    0 |  y(t+1) ]
		currentAlignedBlock <<= b_max - y;
		// [ b_max - y  (t)|  y(t+1) ]
		currentAlignedBlock |= (upperThisBlockPart >> y);

		assert (0 <= b - b_low && other.size() > b - b_low);
		const unsigned long otherBlock = other.bits.m_bits[b - b_low];
		pair<bool, size_t> comp = compareAlignedBlocks(currentAlignedBlock, otherBlock);
		allDimSame = comp.first;
		longestCommonPrefix += comp.second;

		// [  y(t)  |  0    ...     0]
		currentAlignedBlock = lowerThisBlockPart;
	}

	longestCommonPrefix /= DIM;
	assert (longestCommonPrefix >= 0 && longestCommonPrefix <= lsbRange / DIM);
	assert (!allDimSame || other.getBitLength() == longestCommonPrefix);
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
vector<unsigned long> MultiDimBitset<DIM>::toLongs() const {

		vector<unsigned long> numericalValues(DIM, 0);
		const size_t bitLength = bits.size() / DIM;
		for (size_t i = 0; i < bitLength; ++i) {
			const size_t currentBitDepth = i / DIM;
			for (size_t d = 0; d < DIM; ++d) {
				if (bits[i * DIM + d]) {
					numericalValues[d] += 1 << (bitLength - currentBitDepth - 1);
				}
			}
		}

		return numericalValues;
}

template <unsigned int DIM>
unsigned long MultiDimBitset<DIM>::interleaveBits(const size_t msbIndex) const {
		assert (bits.size() >= DIM * (msbIndex + 1));

		// TODO use blockwise operation
		unsigned long hcAddress;
		// the index addresses bits in descending order but internally bits are stored ascendingly
		const size_t lsbStartIndex = bits.size() - DIM * (msbIndex + 1);
		const size_t lsbEndIndex = lsbStartIndex + DIM;
		const size_t b_max = bits.bits_per_block;
		const size_t startBlock = lsbStartIndex / b_max;
		const size_t endBlock = lsbEndIndex / b_max;
		assert (endBlock - startBlock <= 1);

		const unsigned long block = bits.m_bits[startBlock];
		const size_t lower = lsbStartIndex % b_max;

		if (startBlock == endBlock || lsbEndIndex % b_max == 0) {
			const unsigned long maskForInterleaved = (1uL << DIM) - 1uL;
			hcAddress = (block >> lower) & maskForInterleaved;
		} else {
			const size_t upper = lsbEndIndex % b_max;
			const unsigned long maskForStartBlock = ULONG_MAX << lower;
			const unsigned long maskForEndBlock = (1uL << upper) - 1;
			const unsigned long secondBlock = bits.m_bits[endBlock];
			const unsigned long shifSecond = (startBlock + 1) * b_max - lsbStartIndex;
			assert (shifSecond < DIM && shifSecond > 0);
			hcAddress = ((block & maskForStartBlock) >> lower) | ((secondBlock & maskForEndBlock) << shifSecond);
		}

		assert (hcAddress < (1ul<<DIM));
		return hcAddress;
}

template <unsigned int DIM>
void MultiDimBitset<DIM>::removeHighestBits(const size_t nBitsToRemove) {
		if (nBitsToRemove == 0) {
			return;
		}

		// simply cut off the highest bits
		const size_t initialSize = bits.size();
		bits.resize(initialSize - nBitsToRemove * DIM);

		assert (bits.size() == initialSize - (nBitsToRemove * DIM));
}

template <unsigned int DIM>
void MultiDimBitset<DIM>::removeHighestBits(const size_t nBitsToRemove, MultiDimBitset<DIM>* resultTo) const {
	assert (resultTo);
	assert (this->getDim() == resultTo->getDim());
	assert (this->size() >= nBitsToRemove * DIM);
	assert (resultTo->bits.empty());

	const auto initialSize = bits.size();
	if (nBitsToRemove * DIM == initialSize) {
		// no need to remove all bits
		return;
	}

	// copy all blocks before the block to be cut
	const auto remainingBits = initialSize - nBitsToRemove * DIM;
	const auto cutBlockIndex = remainingBits / bits.bits_per_block;
	for (size_t b = 0; b < cutBlockIndex + 1; ++b) {
		unsigned long block = bits.m_bits[b];
		resultTo->bits.append(block);
	}
	resultTo->bits.resize(remainingBits);

	// removed further bits by simply not copying them
	assert (resultTo->bits.size() == remainingBits);
	assert (resultTo->bits.size() % DIM == 0);
}

template <unsigned int DIM>
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
}

template <unsigned int DIM>
void MultiDimBitset<DIM>::duplicateHighestBits(const size_t nBitsToDuplicate,
		MultiDimBitset<DIM>* resultTo) const {
	assert (resultTo);
	assert (resultTo->bits.empty());
	assert (resultTo->getDim() == DIM);
	assert (bits.size() >= DIM * nBitsToDuplicate);

	if (nBitsToDuplicate == 0) {
		return;
	}

	// duplicate:       <------------------------------------------------>
	// this:      | v_1 \   r_1     | , | v_2 \   r_2     | , | v_3 \ r_3 \ free|
	//            | vL  \    rL     | , |-----\-----------| , |-----\-----------|
	// result:    |   r_1    \ v_2  | , |   r_2     \ v_3 | , |  r_3 \    free  |

	const size_t initialSize = bits.size();
	const size_t neglectedLSBBits = initialSize - DIM * nBitsToDuplicate;
	const size_t startCopyBlock = neglectedLSBBits / bits.bits_per_block;
	const size_t lastBlockIndex = initialSize / bits.bits_per_block;
	const unsigned long vL = neglectedLSBBits % bits.bits_per_block;
	const unsigned long rL = bits.bits_per_block - vL;
	const unsigned long vMask = (1uL << vL) - 1;
	const unsigned long rMask = ULLONG_MAX << vL;
	assert (vL + rL == bits.bits_per_block && ((vMask ^ rMask) == ULLONG_MAX));

	unsigned long sourceBlock = this->bits.m_bits[startCopyBlock];
	unsigned long r = (sourceBlock & rMask) >> vL;
	unsigned long currentBlock = r;
	// add current value to free space in current block and set remainder to next block
	for (size_t b = startCopyBlock + 1; b < lastBlockIndex + 1; ++b) {
		const unsigned long sourceBlock = this->bits.m_bits[b];
		const unsigned long v = (sourceBlock & vMask) << rL;
		const unsigned long r = (sourceBlock & rMask) >> vL;
		currentBlock |= v;
		resultTo->bits.append(currentBlock);
		currentBlock = r;
	}
	resultTo->bits.append(currentBlock);
	resultTo->bits.resize(DIM * nBitsToDuplicate);

	// TODO simply set the length of bits instead of resizing because no data is lost
	assert (resultTo->size() == DIM * nBitsToDuplicate);
}

// find the longest common prefix starting at the msbStartIndex and sets it to the result to reference
template <unsigned int DIM>
size_t MultiDimBitset<DIM>::calculateLongestCommonPrefix(const size_t msbStartIndex,
		MultiDimBitset<DIM>* other, MultiDimBitset<DIM>* resultTo) const {

	assert (other && resultTo);
	assert (DIM == other->getDim() && DIM == resultTo->getDim());
	assert (this->getBitLength() - msbStartIndex == other->getBitLength());

	pair<bool,size_t> comparison = this->compareTo(msbStartIndex, msbStartIndex + other->inlineBitLength(), *other);
	const size_t prefixLength = comparison.second;
	assert (prefixLength <= other->getBitLength());
	other->duplicateHighestBits(prefixLength, resultTo);

	assert(resultTo->size() == prefixLength * DIM);
	return prefixLength;
}

#endif /* SRC_UTIL_MULTIDIMBITSET_H_ */
