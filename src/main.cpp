#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"
#include "PlotUtil.cpp"

#define BIT_LENGTH 	8
#define ENTRY_DIM 	2

using namespace std;

int mainSimpleExample() {
	// TODO: add range query

	vector<int> e1Values { 10, 5 };
	vector<int> e2Values { 11, 12 };
	vector<int> e3Values { 60, 16 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);

	PHTree* phtree = new PHTree(ENTRY_DIM, BIT_LENGTH);
	phtree->insert(e1);
	cout << *phtree << endl;

	phtree->insert(e2);
	phtree->lookup(e1);
	phtree->lookup(e3);
	cout << *phtree << endl;

	phtree->insert(e3);
	phtree->lookup(e3);
	cout << *phtree << endl;

	delete phtree;
	delete e1;
	delete e2;

	return 0;
}

int main(int argc, char* argv[]) {

	for (int i = 0; i < argc; i++) {
		cout << "argv[" << i << "]= " << argv[i] << endl;
	}

	string debug = "debug";
	string plot = "plot";

	if (argc != 2 || debug.compare(argv[1]) == 0) {
		return mainSimpleExample();
	} else if (plot.compare(argv[1]) == 0) {
		plotAverageInsertTimePerDimension();
		plotTimeSeriesOfInserts();
		return 0;
	} else {
		cerr << "Missing command line argument!" << endl;
		return 1;
	}
};
