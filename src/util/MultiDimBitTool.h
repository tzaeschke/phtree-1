/*
 * MultiDimBitTool.h
 *
 *  Created on: Mar 16, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_MULTIDIMBITTOOL_H_
#define SRC_UTIL_MULTIDIMBITTOOL_H_

#include <vector>
#include "boost/dynamic_bitset.hpp"
#include "Entry.h"

class MultiDimBitTool {
public:

	static unsigned long bitsetToLong(const boost::dynamic_bitset<>* bitset);

	static std::vector<unsigned long>* bitsetsToLongs(boost::dynamic_bitset<>* bitsets, size_t dim);

	static boost::dynamic_bitset<> longToBitset(unsigned long value, size_t bitLength);

	static void longsToBitsets(boost::dynamic_bitset<>& target,
			const std::vector<unsigned long>& values, size_t bitLength, size_t dim);

	static unsigned long interleaveBits(const unsigned int index, const Entry* e);

	static unsigned long interleaveBits(const unsigned int index, size_t dim,
			const boost::dynamic_bitset<>* values);

	static void removeFirstBits(const unsigned int nBitsToRemove,
			const size_t dim,
			boost::dynamic_bitset<>* values);

	static void removeFirstBits(const unsigned int nBitsToRemove,
			const size_t dim,
			const boost::dynamic_bitset<> *valuesFrom,
			boost::dynamic_bitset<>* valuesTo);

	static void pushBitsToBack(size_t dim,
			boost::dynamic_bitset<> *valuesToPushTo,
			const boost::dynamic_bitset<> *valuesToAdd);

	static void pushValueToBack(boost::dynamic_bitset<> *pushTo, size_t dim,
			unsigned long newValue);

	static void duplicateFirstBits(const unsigned int nBitsToDuplicate,
			const size_t dim,
			const boost::dynamic_bitset<>* from,
			boost::dynamic_bitset<>* to);

	static unsigned int setLongestCommonPrefix(boost::dynamic_bitset<>* entryToSetTo,
			const size_t dim,
			const unsigned int startIndex,
			const boost::dynamic_bitset<>* entry1,
			const boost::dynamic_bitset<>* entry2);
	static Entry createEntryFrom(const size_t dim,
			const boost::dynamic_bitset<>* value1,
			unsigned long hcAddress,
			const boost::dynamic_bitset<>* value2,
			int id);

};

#endif /* SRC_UTIL_MULTIDIMBITTOOL_H_ */
