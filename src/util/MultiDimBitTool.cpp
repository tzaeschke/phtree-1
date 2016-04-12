/*
 * MultiDimBitTool.cpp
 *
 *  Created on: Mar 16, 2016
 *      Author: max
 */

#include "MultiDimBitTool.h"
#include <assert.h>
#include "../Entry.h"

using namespace std;

unsigned long MultiDimBitTool::bitsetToLong(const vector<bool>* bitset) {
	assert (bitset);

	unsigned long numericalValue = 0;
	for (size_t bit = 0; bit < bitset->size(); ++bit) {
		if ((*bitset)[bit]) {
			numericalValue += 1 << (bitset->size() - bit - 1);
		}
	}

	return numericalValue;
}

vector<unsigned long>* MultiDimBitTool::bitsetsToLongs(vector<bool>* bitsets, size_t dim) {
	assert (bitsets);
	assert (bitsets->size() % dim == 0);

	vector<unsigned long>* numericalValues = new vector<unsigned long>(dim, 0);
	const size_t bitLength = bitsets->size() / dim;
	for (size_t i = 0; i < bitLength; ++i) {
		const size_t currentBitDepth = i / dim;
		for (size_t d = 0; d < dim; ++d) {
			if ((*bitsets)[i * dim + d]) {
				(*numericalValues)[d] += 1 << (bitLength - currentBitDepth - 1);
			}
		}
	}

	return numericalValues;
}

void MultiDimBitTool::duplicateFirstBits(const unsigned int nBitsToDuplicate, const size_t dim,
		const vector<bool>* from,
		vector<bool>* to) {
	assert (from && to);
	assert (to->empty());
	assert (from->size() % dim == 0);
	assert (from->size() >= dim * nBitsToDuplicate);

	to->reserve(nBitsToDuplicate * dim);
	for (size_t i = 0; i < nBitsToDuplicate * dim; ++i) {
		to->push_back((*from)[i]);
	}

	assert (to->size() == dim * nBitsToDuplicate);
}

void MultiDimBitTool::pushBitsToBack(size_t dim, vector<bool> *valuesToPushTo,
		const vector<bool> *valuesToAdd) {
	assert (valuesToAdd && valuesToPushTo);
	assert (valuesToPushTo->size() % dim == 0);
	assert (valuesToAdd->size() % dim == 0);
	const size_t initialSize = valuesToPushTo->size();

	valuesToPushTo->reserve(initialSize + valuesToAdd->size());
	for (size_t i = 0; i < valuesToAdd->size(); ++i) {
		valuesToPushTo->push_back((*valuesToAdd)[i]);
	}

	assert (valuesToPushTo->size() == initialSize + valuesToAdd->size());
	assert (valuesToPushTo->capacity() == valuesToPushTo->size());
}

Entry MultiDimBitTool::createEntryFrom(size_t dim, const vector<bool>* prefix,
		unsigned long hcAddress,
		const vector<bool>* suffix, int id) {

	assert (prefix && suffix);
	assert (prefix->size() % dim == 0);
	assert (suffix->size() % dim == 0);

	vector<bool> valuesAppended(1);
	valuesAppended.reserve(prefix->size() + dim + suffix->size());
	vector<bool> addressConverted = longToBitset(hcAddress, dim);

	for (size_t i = 0; i < prefix->size(); ++i) {
		valuesAppended.push_back((*prefix)[i]);
	}
	for (size_t i = 0; i < dim; ++i) {
		valuesAppended.push_back(addressConverted[i]);
	}
	for (size_t i = 0; i < suffix->size(); ++i) {
		valuesAppended.push_back((*suffix)[i]);
	}

	Entry entry(valuesAppended, dim, id);
	assert (entry.values_.size() == prefix->size() + dim + suffix->size());
	return entry;
}

vector<bool> MultiDimBitTool::longToBitset(unsigned long value, size_t bitLength) {
	vector<bool> convertedValue(bitLength);
	for (size_t i = 0; i < bitLength; i++) {
		// extract j-th least segnificant bit from int
		int lsbIndex = bitLength - i - 1;
		bool bit = ((value & (1 << lsbIndex)) >> lsbIndex) == 1;
		convertedValue[i] = bit;
	}
	return convertedValue;
}

void MultiDimBitTool::longsToBitsets(vector<bool>& target,
		const vector<long>& values, size_t bitLength, size_t dim) {
	assert (target.empty());

	// TODO rewrite to directly convert the values
	target.resize(bitLength * dim);
	target.shrink_to_fit();
	for (size_t d = 0; d < dim; ++d) {
		vector<bool> entry = longToBitset(values[d], bitLength);
		for (size_t i = 0; i < bitLength; ++i) {
			target[dim * i + d] = entry[i];
		}
	}

	assert (target.size() == dim * bitLength);
}


void MultiDimBitTool::pushValueToBack(vector<bool> *pushTo, size_t dim, unsigned long newValue) {
	assert (pushTo);
	assert (pushTo->size() % dim == 0);
	vector<bool> convertedValue = longToBitset(newValue, dim);
	assert (convertedValue.size() == dim);
	const size_t initialSize = pushTo->size();

	pushTo->reserve(initialSize + dim);
	for (size_t i = 0; i < dim; ++i) {
		pushTo->push_back(convertedValue[i]);
	}

	assert (pushTo->size() == initialSize + dim);
	assert (pushTo->capacity() == pushTo->size());
}

unsigned long MultiDimBitTool::interleaveBits(const unsigned int index, const Entry* e) {
	assert (e);
	return interleaveBits(index, e->dim_, &(e->values_));
}

unsigned long MultiDimBitTool::interleaveBits(const unsigned int index, size_t dim,
		const vector<bool>* values) {
	assert (values);
	assert (values->size() % dim == 0);
	assert (values->size() >= dim * (index + 1));

	unsigned long hcAddress = 0;
	const size_t max = dim - 1;
	const size_t startIndex = dim * index;
	for (size_t i = 0; i < dim; ++i) {
		hcAddress |= (*values)[startIndex + i] << (max - i);
	}

	assert (hcAddress < 1<<dim);
	return hcAddress;
}

void MultiDimBitTool::removeFirstBits(unsigned int nBitsToRemove, size_t dim,
		vector<bool>* values) {
	assert (values);

	const size_t initialSize = values->size();
	values->erase(values->begin(), values->begin() + (nBitsToRemove * dim));

	assert (values->size() == initialSize - (nBitsToRemove * dim));
}

void MultiDimBitTool::removeFirstBits(unsigned int nBitsToRemove, size_t dim,
		const vector<bool>* valuesFrom,
		vector<bool>* valuesTo) {
	assert (valuesFrom && valuesTo);
	assert (valuesFrom->size() % dim == 0);
	assert (valuesFrom->size() >= nBitsToRemove * dim);
	assert (valuesTo->empty());

	const size_t startIndex = nBitsToRemove * dim;
	const size_t resultingSize = valuesFrom->size() - startIndex;
	valuesTo->reserve(nBitsToRemove * dim);
	for (size_t i = 0; i < resultingSize; ++i) {
		valuesTo->push_back((*valuesFrom)[startIndex + i]);
	}

	assert (valuesTo->size() == valuesFrom->size() - (dim * nBitsToRemove));
}

unsigned int MultiDimBitTool::setLongestCommonPrefix(vector<bool>* entryToSetTo,
		size_t dim,
		unsigned int startIndexEntry1,
		const vector<bool>* entry1,
		const vector<bool>* entry2) {

	const size_t offset = dim * startIndexEntry1;
	assert (entry1 && entry2 && entryToSetTo);
	assert (entry1->size() % dim == 0);
	assert (entry2->size() % dim == 0);
	assert (entry1->size() >= offset);
	assert (entryToSetTo->empty());

	bool allDimSame = true;
	// set the prefix length to the maximum possible value in case it is equal to entry1 after the index
	size_t prefixLength = (entry1->size() - offset) / dim;
	for (size_t i = offset; allDimSame && i < entry1->size(); ++i) {
		allDimSame = (*entry1)[i] == (*entry2)[i - offset];

		if (!allDimSame) {
			prefixLength = (i - offset) / dim;
		}
	}

	const size_t newPrefixSize = dim * prefixLength;
	entryToSetTo->reserve(newPrefixSize);
	for (int i = 0; i < newPrefixSize; ++i) {
		entryToSetTo->push_back((*entry2)[i]);
	}

	assert (entryToSetTo->size() == newPrefixSize);
	return prefixLength;
}
