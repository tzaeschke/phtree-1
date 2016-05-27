/*
 * FileInputUtil.h
 *
 *  Created on: Mar 15, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_FILEINPUTUTIL_H_
#define SRC_UTIL_FILEINPUTUTIL_H_

#include <vector>

class FileInputUtil {
public:
	// parses the file at the given location in the format 'ulong, ulong, ulong, ...\n...'
	template <unsigned int DIM, unsigned int WIDTH>
	static std::vector<vector<unsigned long>>* readEntries(std::string fileLocation);
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
vector<vector<unsigned long>>* FileInputUtil::readEntries(string fileLocation) {

	ifstream myfile (fileLocation);
	vector<vector<unsigned long>>* result = new vector<vector<unsigned long>>();
	if (myfile.is_open()) {
		int id = 0;
		while (!myfile.eof()) {
			vector<unsigned long> values = getNextLineTokens(myfile);
			if (!values.empty()) {
				result->push_back(values);
			}
		}
	} else {
		throw runtime_error("cannot open the file " + fileLocation);
	}

	return result;
}

#endif /* SRC_UTIL_FILEINPUTUTIL_H_ */
