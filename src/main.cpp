#include <iostream>
#include <string>
#include <vector>

#include "Entry.h"
#include "PHTree.h"

#define BIT_LENGTH 8
using namespace std;

int main() {

	// TODO: add range query

	vector<int> e1Values { 10, 5 };
	vector<int> e2Values { 11, 12 };
	vector<int> e3Values { 60, 16 };
	vector<int> e4Values { 1, 3 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);
	Entry* e4 = new Entry(e4Values, BIT_LENGTH);

	PHTree* phtree = new PHTree(2, BIT_LENGTH);
	phtree->insert(e1);
	cout << *phtree << endl;

	phtree->insert(e2);
	phtree->lookup(e1);
	phtree->lookup(e3);
	cout << *phtree << endl;

	phtree->insert(e3);
	phtree->lookup(e3);
	cout << *phtree << endl;

	phtree->insert(e4);
	phtree->lookup(e4);
	cout << *phtree << endl;

	delete phtree;
	delete e1;
	delete e2;
	delete e3;
	delete e4;

	return 0;
}
;
