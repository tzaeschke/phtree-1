/*
 * FileInputUtil.cpp
 *
 *  Created on: Mar 15, 2016
 *      Author: max
 */

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

vector<Entry*> FileInputUtil::readEntries(string fileLocation, size_t bitLength) {

	ifstream myfile (fileLocation);
		std::vector<Entry*>   result;
	if (myfile.is_open()) {
		int id = 0;
		while (!myfile.eof()) {
			vector<unsigned long> values = getNextLineTokens(myfile);
			if (!values.empty()) {
				Entry* entry = new Entry(values, bitLength, id++);
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
