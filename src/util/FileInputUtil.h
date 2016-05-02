/*
 * FileInputUtil.h
 *
 *  Created on: Mar 15, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_FILEINPUTUTIL_H_
#define SRC_UTIL_FILEINPUTUTIL_H_

#include <vector>

template <unsigned int DIM>
class Entry;

class FileInputUtil {
public:
	// parses the file at the given location in the format 'int, int, int, ...\n...'
	template <unsigned int DIM>
	static std::vector<Entry<DIM>*> readEntries(std::string fileLocation, size_t bitLength);
};

#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <string>
#include <assert.h>
#include <stdexcept>
#include "Entry.h"
#include "util/FileInputUtil.h"

using namespace std;

inline vector<unsigned long> getNextLineTokens(ifstream& stream) {
	string line;
	getline(stream, line);
	stringstream lineStream(line);
	string cell;
	vector<unsigned long> tokens;

	while (getline(lineStream, cell, ',')) {
		unsigned long parsedToken = stoi(cell);
		tokens.push_back(parsedToken);
	}

	return tokens;
}

template <unsigned int DIM>
vector<Entry<DIM>*> FileInputUtil::readEntries(string fileLocation, size_t bitLength) {

	ifstream myfile (fileLocation);
		std::vector<Entry<DIM>*>   result;
	if (myfile.is_open()) {
		int id = 0;
		while (!myfile.eof()) {
			vector<unsigned long> values = getNextLineTokens(myfile);
			if (!values.empty()) {
				Entry<DIM>* entry = new Entry<DIM>(values, bitLength, id++);
				assert (entry->getBitLength() == bitLength);
				assert (result.empty() || result.at(0)->getDimensions() == entry->getDimensions());
				result.push_back(entry);
			}
		}
	} else {
		throw runtime_error("cannot open the file " + fileLocation);
	}

	return result;
}

#endif /* SRC_UTIL_FILEINPUTUTIL_H_ */
