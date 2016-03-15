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
			Entry* entry = new Entry(values, bitLength);
			result.push_back(entry);
			cout << *entry << endl;
		}
	}

	return result;


	/*ifstream myReadFile;
	regex regex("(\d+)(,\s*\d+)*");
	myReadFile.open(fileLocation.c_str());
	char buffer[1000];
	smatch sm;

	vector<Entry*>* entries = new vector<Entry*>();
	if (myReadFile.is_open()) {
		while (!myReadFile.eof()) {
			myReadFile >> buffer;
			regex_match(buffer, sm, regex);
			for (unsigned i = 0; i < sm.size(); ++i) {
				cout << sm[i];
			}

		}
	}
	myReadFile.close();

	return *entries;*/
}
