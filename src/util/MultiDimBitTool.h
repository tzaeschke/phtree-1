/*
 * MultiDimBitTool.h
 *
 *  Created on: Mar 16, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_MULTIDIMBITTOOL_H_
#define SRC_UTIL_MULTIDIMBITTOOL_H_

#include <vector>
#include "../Entry.h"

class MultiDimBitTool {
public:

	static unsigned long bitsetToLong(const std::vector<bool>* bitset);

	static std::vector<unsigned long>* bitsetsToLongs(std::vector<bool>* bitsets, size_t dim);

	static std::vector<bool> longToBitset(unsigned long value, size_t bitLength);

	static void longsToBitsets(std::vector<bool>& target,
			const std::vector<long>& values, size_t bitLength, size_t dim);

	static unsigned long interleaveBits(const unsigned int index, const Entry* e);

	static unsigned long interleaveBits(const unsigned int index, size_t dim,
			const std::vector<bool>* values);

	static void removeFirstBits(const unsigned int nBitsToRemove,
			const size_t dim,
			std::vector<bool> *values);

	static void removeFirstBits(const unsigned int nBitsToRemove,
			const size_t dim,
			const std::vector<bool> *valuesFrom,
			std::vector<bool>* valuesTo);

	static void pushBitsToBack(size_t dim,
			std::vector<bool> *valuesToPushTo,
			const std::vector<bool> *valuesToAdd);

	static void pushValueToBack(std::vector<bool> *pushTo, size_t dim,
			unsigned long newValue);

	static void duplicateFirstBits(const unsigned int nBitsToDuplicate,
			const size_t dim,
			const std::vector<bool>* from,
			std::vector<bool>* to);

	static unsigned int setLongestCommonPrefix(std::vector<bool>* entryToSetTo,
			const size_t dim,
			const unsigned int startIndex,
			const std::vector<bool>* entry1,
			const std::vector<bool>* entry2);
	static Entry createEntryFrom(const size_t dim,
			const std::vector<bool>* value1,
			unsigned long hcAddress,
			const std::vector<bool>* value2,
			int id);

};

#endif /* SRC_UTIL_MULTIDIMBITTOOL_H_ */
