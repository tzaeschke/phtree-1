#include <iostream>
#include <string>
#include <vector>

#include "Entry.h"
#include "PHTree.h"

#define BIT_LENGTH 8
using namespace std;

int main() {

	// TODO: duplicate entry before removing bits to store them as a suffix so the original object is preserved
	// TODO: add << for PH tree class
	// TODO: use algorithm pattern to elevate functionality into Node class
	// TODO: add LHC
	// TODO: add range query

	vector<int> e1Values { 10, 5 };
	vector<int> e2Values { 11, 12 };
	vector<int> e3Values { 60, 16 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);
	Entry* e1Duplicate = new Entry(e1Values, BIT_LENGTH);
	Entry* e3Duplicate = new Entry(e3Values, BIT_LENGTH);

	PHTree* phtree = new PHTree(2, BIT_LENGTH);
	phtree->insert(e1);

	phtree->insert(e2);
	phtree->lookup(e1Duplicate);
	phtree->lookup(e3Duplicate);

	phtree->insert(e3);
	phtree->lookup(e3Duplicate);

	delete phtree;
	delete e1;
	delete e2;

	return 0;
}
;
