#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"
#include "PlotUtil.h"
#include "visitors/CountNodeTypesVisitor.h"

#define BIT_LENGTH 	8
#define ENTRY_DIM 	2

using namespace std;

int mainSimpleExample() {
	// TODO: add range query

	vector<int> e1Values { 10, 5 };
	vector<int> e2Values { 11, 12 };
	vector<int> e3Values { 60, 16 };
	vector<int> e4Values { 1, 3 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);
	Entry* e4 = new Entry(e4Values, BIT_LENGTH);

	CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
	PHTree* phtree = new PHTree(ENTRY_DIM, BIT_LENGTH);
	phtree->insert(e1);
	phtree->accept(visitor);
	cout << *phtree;
	cout << *visitor << endl;

	phtree->insert(e2);
	phtree->lookup(e1);
	phtree->lookup(e3);
	visitor->reset();
	phtree->accept(visitor);
	cout << *phtree;
	cout << *visitor << endl;

	phtree->insert(e3);
	phtree->lookup(e3);
	visitor->reset();
	phtree->accept(visitor);
	cout << *phtree;
	cout << *visitor << endl;

	phtree->insert(e4);
	phtree->lookup(e4);
	visitor->reset();
	phtree->accept(visitor);
	cout << *phtree;
	cout << *visitor << endl;

	delete phtree;
	delete e1;
	delete e2;
	delete e3;
	delete e4;

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
		PlotUtil::plotAverageInsertTimePerDimension();
		PlotUtil::plotTimeSeriesOfInserts();
		return 0;
	} else {
		cerr << "Missing command line argument!" << endl;
		return 1;
	}
};
