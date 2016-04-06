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
	unsigned long numericalValue = 0;
	for (size_t bit = 0; bit < bitset->size(); ++bit) {
		if (bitset->at(bit)) {
			numericalValue += 1 << (bitset->size() - bit - 1);
		}
	}

	return numericalValue;
}

vector<unsigned long>* MultiDimBitTool::bitsetsToLongs(vector<vector<bool>>* bitsets) {
	vector<unsigned long>* numericalValues = new vector<unsigned long>(bitsets->size());
	for (size_t i = 0; i < bitsets->size(); ++i) {
		numericalValues->at(i) = bitsetToLong(&(bitsets->at(i)));
	}

	return numericalValues;
}

void MultiDimBitTool::duplicateFirstBits(unsigned int nBitsToDuplicate,
		const vector<vector<bool>>* from,
		vector<vector<bool>>* to) {
	for (size_t i = 0; i < from->size(); i++) {
		for (size_t j = 0; j < nBitsToDuplicate; j++) {
			to->at(i).push_back(from->at(i).at(j));
		}
	}

//	assert (to->size() == dim_);
	assert (to->at(0).size() == nBitsToDuplicate);
}

void MultiDimBitTool::pushBitsToBack(vector<vector<bool>> *valuesToPushTo,
		const vector<vector<bool>> *valuesToAdd) {
	assert (valuesToAdd->size() == valuesToPushTo->size());
	for (size_t dim = 0; dim < valuesToAdd->size(); ++dim) {
		for (size_t bit = 0; bit < valuesToAdd->at(dim).size(); ++bit) {
			valuesToPushTo->at(dim).push_back(valuesToAdd->at(dim).at(bit));
		}
	}
}

Entry MultiDimBitTool::createEntryFrom(const vector<vector<bool>>* prefix,
		unsigned long hcAddress,
		const vector<vector<bool>>* suffix, int id) {
	assert (prefix->size() == suffix->size());
	assert (prefix->size() > 0);

	size_t dim = prefix->size();
	size_t prefixLength = prefix->at(0).size();
	size_t suffixLength = suffix->at(0).size();
	vector<vector<bool>> valuesAppended(dim);
	vector<bool> addressConverted = longToBitset(hcAddress, dim);
	for (size_t d = 0; d < dim; ++d) {
		assert (prefixLength == prefix->at(d).size() && suffixLength == suffix->at(d).size());

		valuesAppended.at(d) = vector<bool>(prefixLength + 1 + suffixLength);
		for (size_t bit = 0; bit < prefixLength; ++bit) {
			valuesAppended.at(d).at(bit) = prefix->at(d).at(bit);
		}
		valuesAppended.at(d).at(prefixLength) = addressConverted.at(d);
		for (size_t bit = 0; bit < suffixLength; ++bit) {
			valuesAppended.at(d).at(prefixLength + 1 + bit) = suffix->at(d).at(bit);
		}
	}

	Entry entry(valuesAppended, id);
	return entry;
}

vector<bool> MultiDimBitTool::longToBitset(unsigned long value, size_t bitLength) {
	vector<bool> convertedValue(bitLength);
	for (size_t i = 0; i < bitLength; i++) {
		// extract j-th least segnificant bit from int
		int lsbIndex = bitLength - i - 1;
		bool bit = ((value & (1 << lsbIndex)) >> lsbIndex) == 1;
		convertedValue.at(i) = bit;
	}
	return convertedValue;
}

void MultiDimBitTool::longsToBitsets(vector<vector<bool>>& target,
		const vector<long>& values, size_t bitLength) {
	assert (target.size() == values.size());

	for (size_t i = 0; i < values.size(); ++i) {
		target.at(i) = longToBitset(values.at(i), bitLength);
	}
}


void MultiDimBitTool::pushValueToBack(vector<vector<bool>> *pushTo, unsigned long newValue) {
	assert (pushTo->size() > 0);
	size_t dim = pushTo->size();
	vector<bool> convertedValue = longToBitset(newValue, dim);
	assert (convertedValue.size() == pushTo->size());
	for (size_t d = 0; d < dim; ++d) {
		pushTo->at(d).push_back(convertedValue.at(d));
	}
}

long MultiDimBitTool::interleaveBits(unsigned int index, const Entry* e) {
	return interleaveBits(index, &(e->values_));
}

long MultiDimBitTool::interleaveBits(unsigned int index,
		const vector<vector<bool>>* values) {
	long hcAddress = 0;
	size_t max = values->size() - 1;
	for (size_t value = 0; value < values->size(); value++) {
		hcAddress |= (*values)[value][index] << (max - value);
	}
	return hcAddress;
}

void MultiDimBitTool::removeFirstBits(unsigned int nBitsToRemove,
		vector<vector<bool>>* values) {
	for (size_t i = 0; i < values->size(); i++) {
			values->at(i).erase(values->at(i).begin(),
					values->at(i).begin() + nBitsToRemove);
	}

//	assert (values->size() == dim_);
}

void MultiDimBitTool::removeFirstBits(unsigned int nBitsToRemove,
		const vector<vector<bool>>* valuesFrom,
		vector<vector<bool>>* valuesTo) {
//	assert (valuesTo->size() == dim_);
	assert (valuesTo->at(0).empty());

	for (size_t valueIndex = 0; valueIndex < valuesFrom->size(); valueIndex++) {
		size_t newLength = (*valuesFrom)[valueIndex].size() - nBitsToRemove;
		valuesTo->at(valueIndex) = vector<bool>(newLength);
		for (size_t bit = 0; bit < newLength; bit++) {
			bool copiedBit = valuesFrom->at(valueIndex).at(nBitsToRemove + bit);
			valuesTo->at(valueIndex).at(bit) = copiedBit;
			//value->at(bit) = copiedBit;
		}
	}

//	assert (valuesTo->size() == dim_); // should retain the dimension
	assert (valuesTo->at(0).size() == valuesFrom->at(0).size() - nBitsToRemove);
}

unsigned int MultiDimBitTool::setLongestCommonPrefix(vector<vector<bool>>* entryToSetTo,
		unsigned int startIndexEntry1,
		const vector<vector<bool>>* entry1,
		const vector<vector<bool>>* entry2) {

	assert (entry1->size() == entry2->size()
			&& "both entries must have the same dimensions");

	unsigned long dim = entry1->size();
	unsigned long valueLength = entry1->at(0).size();

	assert (dim > 0 && "the entry should have a dimension");
	assert (entry1->at(0).size() > startIndexEntry1
			&& "the first entry must include the given index");
	assert (entryToSetTo->size() == dim
			&& "the prefix must already have the same dimensions as the node");
	assert (entryToSetTo->at(0).empty()
			&& "the prefix must not have been set before");

	bool allDimSame = true;
	size_t prefixLength = 0;
	// TODO (performance) iterate over vector linearly for (dim) { for (bit) {}} to use cache
	for (size_t i = startIndexEntry1; i < valueLength && allDimSame; i++) {
		for (size_t val = 0; val < dim && allDimSame; val++)
			allDimSame = (*entry1).at(val).at(i) == (*entry2).at(val).at(i - startIndexEntry1);

		if (allDimSame)
			prefixLength++;
		for (size_t val = 0; val < dim && allDimSame; val++)
			entryToSetTo->at(val).push_back((*entry1)[val][i]);
	}

	assert (entryToSetTo->size() == dim
			&& "afterwards the prefix should have the same dimensionality as the node");
	assert (entryToSetTo->at(0).size() == prefixLength
			&& "should have set the correct prefix length");

	return prefixLength;
}
