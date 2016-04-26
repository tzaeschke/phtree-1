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
#include "boost/dynamic_bitset.hpp"

class MultiDimBitset {
	friend std::ostream& operator <<(std::ostream &out, const MultiDimBitset &bitset);
	friend class SizeVisitor;
public:
	MultiDimBitset();
	MultiDimBitset(const MultiDimBitset &other);
	MultiDimBitset(const size_t dim);
	MultiDimBitset(std::vector<unsigned long> &values, size_t bitLength, size_t dim);
	virtual ~MultiDimBitset();

	size_t size() const;
	size_t getBitLength() const;
	size_t getDim() const;
	void setDim(size_t dim);

	void clear();
	bool operator ==(const MultiDimBitset &b) const;
	bool operator !=(const MultiDimBitset &b) const;
	std::ostream& operator <<(std::ostream &out) const;

	std::pair<bool, size_t> compareTo(size_t fromIndex, size_t toIndex, const MultiDimBitset &other) const;
	std::vector<unsigned long> toLongs() const;
	unsigned long interleaveBits(const size_t index) const;
	void removeHighestBits(const size_t nBitsToRemove);
	void removeHighestBits(const size_t nBitsToRemove, MultiDimBitset* resultTo) const;
	void pushBitsToBack(const MultiDimBitset* source);
	void pushValueToBack(unsigned long newValue);
	void duplicateHighestBits(const size_t nBitsToDuplicate, MultiDimBitset* resultTo) const;
	size_t calculateLongestCommonPrefix(const size_t startIndex, MultiDimBitset* other, MultiDimBitset* resultTo) const;

private:
	size_t dim_;

	// consecutive storage of the multidimensional values in bit representation
	boost::dynamic_bitset<> bits;

	static boost::dynamic_bitset<> longToBitset(unsigned long value, size_t bitLength);
	static inline std::pair<bool, size_t> compareAlignedBlocks(const unsigned long b1, const unsigned long b2);
	inline size_t inlineBitLength() const;

};

#endif /* SRC_UTIL_MULTIDIMBITSET_H_ */
