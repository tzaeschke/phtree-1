#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"
#include "util/PlotUtil.h"
#include "visitors/CountNodeTypesVisitor.h"
#include "iterators/RangeQueryIterator.h"

#define BIT_LENGTH 	8
#define ENTRY_DIM 	2

using namespace std;

int mainSimpleExample() {
	// TODO: add range query

	vector<long> e1Values { 10, 5 };
	vector<long> e2Values { 11, 12 };
	vector<long> e3Values { 60, 7 };
	vector<long> e4Values { 1, 3 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);
	Entry* e4 = new Entry(e4Values, BIT_LENGTH);

	CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
	PHTree* phtree = new PHTree(ENTRY_DIM, BIT_LENGTH);
	phtree->insert(e1);
	cout << *phtree;
	phtree->accept(visitor);
	cout << *visitor << endl;

	phtree->insert(e2);
	assert (phtree->lookup(e1));
	assert (!phtree->lookup(e3));
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	phtree->insert(e3);
	assert (phtree->lookup(e3));
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	phtree->insert(e4);
	assert (phtree->lookup(e4));
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	/*cout << "The following entries are in the range (0,0) - (20,20):" << endl;
	RangeQueryIterator* it = phtree->rangeQuery(new Entry({0,0}, BIT_LENGTH), new Entry({20,20}, BIT_LENGTH));
	while (it->hasNext()) {
		Entry entryInRange = it->next();
		cout << entryInRange << endl;
	}*/

	cout << "The following entries are in the range (1,2) - (100,10):" << endl;
	RangeQueryIterator* it = phtree->rangeQuery(new Entry({1,2}, BIT_LENGTH), new Entry( {100,10}, BIT_LENGTH));
	while (it->hasNext()) {
		Entry entryInRange = it->next();
		cout << entryInRange << endl;
	}

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
		PlotUtil::plotAverageInsertTimePerDimension("./plot/data/phtree_java_rand_unique_entries.dat", 32);
//		PlotUtil::plotTimeSeriesOfInserts();
//		PlotUtil::plotAverageInsertTimePerDimensionRandom();
//		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom();
		return 0;
	} else {
		cerr << "Missing command line argument!" << endl << "valid: 'debug', 'plot'";
		return 1;
	}
};
