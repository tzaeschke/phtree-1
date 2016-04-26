/*
 * MultiDimBitset.cpp
 *
 *  Created on: Apr 18, 2016
 *      Author: max
 */

#include "util/MultiDimBitset.h"
#include <string.h>
#include <float.h>
#include <math.h>

using namespace std;

MultiDimBitset::MultiDimBitset() : dim_(0) {
}

MultiDimBitset::MultiDimBitset(const size_t dim) : dim_(dim) {
}

MultiDimBitset::MultiDimBitset(const MultiDimBitset &other) : dim_(other.dim_), bits(other.bits) {

}

MultiDimBitset::MultiDimBitset(std::vector<unsigned long> &values, size_t bitLength, size_t dim) : dim_(dim) {
	// example 2 Dim, 8 Bit:     (   10    ,    5     )
	// binary representation:    (0000 1010, 0000 0101)
	// index:                       12    8    4    0
	// after conversion (mixed): (0000 0000 0110 0110)

		// TODO rewrite to directly convert the values
		bits.resize(bitLength * dim);
		for (size_t d = 0; d < dim; ++d) {
			boost::dynamic_bitset<> entry = longToBitset(values[d], bitLength);
			for (size_t i = 0; i < bitLength; ++i) {
				bits[dim * i + d] = entry[i];
			}
		}

	assert (this->size() == dim * bitLength);

}

boost::dynamic_bitset<> MultiDimBitset::longToBitset(unsigned long value, size_t bitLength) {
	boost::dynamic_bitset<> convertedValue(bitLength);
	for (size_t i = 0; i < bitLength; i++) {
		// extract j-th least segnificant bit from int
		// TODO make use of bitset functions
		bool bit = ((value & (1 << i)) >> i) == 1;
		convertedValue[i] = bit;
	}
	return convertedValue;
}

MultiDimBitset::~MultiDimBitset() {
	bits.clear();
}

bool MultiDimBitset::operator ==(const MultiDimBitset &b) const {
	return this->bits == b.bits;
}

bool MultiDimBitset::operator !=(const MultiDimBitset &b) const {
	return this->bits != b.bits;
}

std::ostream& operator <<(std::ostream &os, const MultiDimBitset &bitset) {
	os << "(";
	// bits are stored in ascending order so they need to be printed in reverse oder
	const size_t bitLength = bitset.getBitLength();
	for (size_t d = 0; d < bitset.dim_; ++d) {
		os << "(";
		for (long i = bitLength - 1; i >= 0; --i) {
			const int bitNumber = (bitset.bits[i * bitset.dim_ + d]) ? 1 : 0;
			os << bitNumber;
		}
		os << ")";
		if (d < bitset.dim_ - 1) {
			os << ", ";
		}
	}

	os << ")";
	return os;
}

size_t MultiDimBitset::size() const {
	return bits.size();
}

size_t MultiDimBitset::getBitLength() const {
	if (dim_ == 0) {
		return 0;
	}

	return bits.size() / dim_;
}

inline size_t MultiDimBitset::inlineBitLength() const {
	return bits.size() / dim_;
}

size_t MultiDimBitset::getDim() const {
	return dim_;
}

void MultiDimBitset::setDim(size_t dim) {
	assert (bits.empty());
	dim_ = dim;
}

void MultiDimBitset::clear() {
	bits.clear();
}

pair<bool, size_t> MultiDimBitset::compareTo(size_t fromMsbIndex, size_t toMsbIndex, const MultiDimBitset &other) const {
	assert (dim_ > 0);

	const size_t upper = dim_ * (inlineBitLength() - fromMsbIndex);
	const size_t lower = dim_ * (inlineBitLength() - toMsbIndex);
	const size_t lsbRange = upper - lower;
	const size_t b_max = bits.bits_per_block;
	assert(lower <= upper);

	if (other.bits.empty() || upper == lower) {
		return pair<bool, size_t>(true, 0);
	}

	assert(this->dim_ == other.dim_);
	assert(this->size() >= upper);
	assert(lsbRange == other.bits.size());
	assert(lsbRange % dim_ == 0);

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

	longestCommonPrefix /= dim_;
	assert (longestCommonPrefix >= 0 && longestCommonPrefix <= lsbRange / dim_);
	assert (!allDimSame || other.getBitLength() == longestCommonPrefix);
	return pair<bool, size_t>(allDimSame, longestCommonPrefix);
}

pair<bool, size_t> MultiDimBitset::compareAlignedBlocks(const unsigned long b1, const unsigned long b2) {
	unsigned long comparison = b1 ^ b2;
	size_t longestCommonPrefix = sizeof (unsigned long) * 8;
	bool allDimSame = comparison == 0;
	if (!allDimSame) {
		longestCommonPrefix = __builtin_clzl(comparison);
	}

	return pair<bool, size_t>(allDimSame, longestCommonPrefix);
}

vector<unsigned long> MultiDimBitset::toLongs() const {

		vector<unsigned long> numericalValues(dim_, 0);
		const size_t bitLength = bits.size() / dim_;
		for (size_t i = 0; i < bitLength; ++i) {
			const size_t currentBitDepth = i / dim_;
			for (size_t d = 0; d < dim_; ++d) {
				if (bits[i * dim_ + d]) {
					numericalValues[d] += 1 << (bitLength - currentBitDepth - 1);
				}
			}
		}

		return numericalValues;
}

unsigned long MultiDimBitset::interleaveBits(const size_t msbIndex) const {
		assert (bits.size() >= dim_ * (msbIndex + 1));

		// TODO use blockwise operation
		unsigned long hcAddress;
		// the index addresses bits in descending order but internally bits are stored ascendingly
		const size_t lsbStartIndex = bits.size() - dim_ * (msbIndex + 1);
		const size_t lsbEndIndex = lsbStartIndex + dim_;
		const size_t b_max = bits.bits_per_block;
		const size_t startBlock = lsbStartIndex / b_max;
		const size_t endBlock = lsbEndIndex / b_max;
		assert (endBlock - startBlock <= 1);

		const unsigned long block = bits.m_bits[startBlock];
		const size_t lower = lsbStartIndex % b_max;

		if (startBlock == endBlock || lsbEndIndex % b_max == 0) {
			const unsigned long maskForInterleaved = (1uL << dim_) - 1uL;
			hcAddress = (block >> lower) & maskForInterleaved;
		} else {
			const size_t upper = lsbEndIndex % b_max;
			const unsigned long maskForStartBlock = ULONG_MAX << lower;
			const unsigned long maskForEndBlock = (1uL << upper) - 1;
			const unsigned long secondBlock = bits.m_bits[endBlock];
			const unsigned long shifSecond = (startBlock + 1) * b_max - lsbStartIndex;
			assert (shifSecond < dim_ && shifSecond > 0);
			hcAddress = ((block & maskForStartBlock) >> lower) | ((secondBlock & maskForEndBlock) << shifSecond);
		}

		assert (hcAddress < (1ul<<dim_));
		return hcAddress;
}

void MultiDimBitset::removeHighestBits(const size_t nBitsToRemove) {
		if (nBitsToRemove == 0) {
			return;
		}

		// simply cut off the highest bits
		const size_t initialSize = bits.size();
		bits.resize(initialSize - nBitsToRemove * dim_);

		assert (bits.size() == initialSize - (nBitsToRemove * dim_));
}

void MultiDimBitset::removeHighestBits(const size_t nBitsToRemove, MultiDimBitset* resultTo) const {
	assert (resultTo);
	assert (this->dim_ == resultTo->dim_);
	assert (this->size() >= nBitsToRemove * dim_);
	assert (resultTo->bits.empty());

	const auto initialSize = bits.size();
	if (nBitsToRemove * dim_ == initialSize) {
		// no need to remove all bits
		return;
	}

	// copy all blocks before the block to be cut
	const auto remainingBits = initialSize - nBitsToRemove * dim_;
	const auto cutBlockIndex = remainingBits / bits.bits_per_block;
	for (size_t b = 0; b < cutBlockIndex + 1; ++b) {
		unsigned long block = bits.m_bits[b];
		resultTo->bits.append(block);
	}
	resultTo->bits.resize(remainingBits);

	// removed further bits by simply not copying them
	assert (resultTo->bits.size() == remainingBits);
	assert (resultTo->bits.size() % dim_ == 0);
}

void MultiDimBitset::pushBitsToBack(const MultiDimBitset* source) {
	assert(source);
	assert(source->dim_ == dim_);
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

void MultiDimBitset::pushValueToBack(unsigned long newValue) {
		boost::dynamic_bitset<> convertedValue = longToBitset(newValue, dim_);
		assert (convertedValue.size() == dim_);
		const size_t initialSize = bits.size();

		bits.resize(initialSize + dim_);
		for (size_t i = 0; i < dim_; ++i) {
			bits[i] = convertedValue[i];
		}

		assert (bits.size() == initialSize + dim_);
}
void MultiDimBitset::duplicateHighestBits(const size_t nBitsToDuplicate,
		MultiDimBitset* resultTo) const {
	assert (resultTo);
	assert (resultTo->bits.empty());
	assert (resultTo->dim_ == dim_);
	assert (bits.size() >= dim_ * nBitsToDuplicate);

	if (nBitsToDuplicate == 0) {
		return;
	}

	// duplicate:       <------------------------------------------------>
	// this:      | v_1 \   r_1     | , | v_2 \   r_2     | , | v_3 \ r_3 \ free|
	//            | vL  \    rL     | , |-----\-----------| , |-----\-----------|
	// result:    |   r_1    \ v_2  | , |   r_2     \ v_3 | , |  r_3 \    free  |

	const size_t initialSize = bits.size();
	const size_t neglectedLSBBits = initialSize - dim_ * nBitsToDuplicate;
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
	resultTo->bits.resize(dim_ * nBitsToDuplicate);

	// TODO simply set the length of bits instead of resizing because no data is lost
	assert (resultTo->size() == dim_ * nBitsToDuplicate);
}

// find the longest common prefix starting at the msbStartIndex and sets it to the result to reference
size_t MultiDimBitset::calculateLongestCommonPrefix(const size_t msbStartIndex,
		MultiDimBitset* other, MultiDimBitset* resultTo) const {

	assert (other && resultTo);
	assert (this->dim_ == other->dim_ && this->dim_ == resultTo->dim_);
	assert (this->getBitLength() - msbStartIndex == other->getBitLength());

	pair<bool,size_t> comparison = this->compareTo(msbStartIndex, msbStartIndex + other->inlineBitLength(), *other);
	const size_t prefixLength = comparison.second;
	assert (prefixLength <= other->getBitLength());
	other->duplicateHighestBits(prefixLength, resultTo);

	assert(resultTo->size() == prefixLength * dim_);
	return prefixLength;
}
