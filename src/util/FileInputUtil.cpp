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
#include "FileInputUtil.h"
#include "../Entry.h"
#include <assert.h>

using namespace std;

FileInputUtil::FileInputUtil() {
}

FileInputUtil::~FileInputUtil() {
}


inline vector<int> getNextLineTokens(ifstream& stream) {
	string line;
	getline(stream, line);
	stringstream lineStream(line);
	string cell;
	vector<int> tokens;

	while (getline(lineStream, cell, ',')) {
		int parsedToken = stoi(cell);
		tokens.push_back(parsedToken);
	}

	return tokens;
}

vector<Entry*> FileInputUtil::readEntries(string fileLocation, size_t bitLength) {

	ifstream myfile (fileLocation);
		std::vector<Entry*>   result;
	if (myfile.is_open()) {
		while (!myfile.eof()) {
			vector<int> values = getNextLineTokens(myfile);
			if (!values.empty()) {
				Entry* entry = new Entry(values, bitLength);
				assert (entry->getBitLength() == bitLength);
				assert (result.empty() || result.at(0)->getDimensions() == entry->getDimensions());
				result.push_back(entry);
			}
		}
	}

	return result;
}
