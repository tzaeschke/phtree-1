/*
 * FileInputUtil.h
 *
 *  Created on: Mar 15, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_FILEINPUTUTIL_H_
#define SRC_UTIL_FILEINPUTUTIL_H_

#include <vector>

template <unsigned int DIM, unsigned int WIDTH>
class Entry;

class FileInputUtil {
public:
	// parses the file at the given location in the format 'int, int, int, ...\n...'
	template <unsigned int DIM, unsigned int WIDTH>
	static std::vector<Entry<DIM, WIDTH>*> readEntries(std::string fileLocation);
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

template <unsigned int DIM, unsigned int WIDTH>
vector<Entry<DIM, WIDTH>*> FileInputUtil::readEntries(string fileLocation) {

	ifstream myfile (fileLocation);
		std::vector<Entry<DIM, WIDTH>*>   result;
	if (myfile.is_open()) {
		int id = 0;
		while (!myfile.eof()) {
			vector<unsigned long> values = getNextLineTokens(myfile);
			if (!values.empty()) {
				Entry<DIM, WIDTH>* entry = new Entry<DIM, WIDTH>(values, id++);
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
