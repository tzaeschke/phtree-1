#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"
#include "util/PlotUtil.h"
#include "util/rdtsc.h"
#include "visitors/CountNodeTypesVisitor.h"
#include "iterators/RangeQueryIterator.h"

#define BIT_LENGTH 	8

using namespace std;

int mainSimpleExample() {
	vector<long> e1Values { 10, 5 };
	vector<long> e2Values { 11, 12 };
	vector<long> e3Values { 60, 7 };
	vector<long> e4Values { 1, 3 };
	Entry* e1 = new Entry(e1Values, BIT_LENGTH);
	Entry* e2 = new Entry(e2Values, BIT_LENGTH);
	Entry* e3 = new Entry(e3Values, BIT_LENGTH);
	Entry* e4 = new Entry(e4Values, BIT_LENGTH);

	CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
	uint64_t sta = RDTSC();
	PHTree* phtree = new PHTree(2, BIT_LENGTH);
	phtree->insert(e1);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	cout << *phtree;
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e2);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e1));
	assert (!phtree->lookup(e3));
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e3);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e3));
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e4);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
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
	}

	cout << "The following entries are in the range (1,2) - (100,10):" << endl;
	RangeQueryIterator* it = phtree->rangeQuery(new Entry({1,2}, BIT_LENGTH), new Entry( {100,10}, BIT_LENGTH));
	while (it->hasNext()) {
		Entry entryInRange = it->next();
		cout << entryInRange << endl;
	}*/

	delete visitor;
	delete phtree;
	delete e1;
	delete e2;
	delete e3;
	delete e4;

	return 0;
}

int main(int argc, char* argv[]) {

	string debug = "debug";
	string plot = "plot";
	string rand = "rand";
	string benchmark = "benchmark";

	if (argc != 2 || debug.compare(argv[1]) == 0) {
		return mainSimpleExample();
	} else if (plot.compare(argv[1]) == 0) {
		PlotUtil::plotAverageInsertTimePerDimension("./plot/data/phtree_java_rand_unique_entries.dat", 32);
//		PlotUtil::plotTimeSeriesOfInserts();
//		PlotUtil::plotAverageInsertTimePerDimensionRandom();
//		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom();
		return 0;
	} else if (rand.compare(argv[1]) == 0) {
		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom();
	} else if (benchmark.compare(argv[1]) == 0) {
		cout << "run a benchmark extracted from the Java implementation with 1M 3D 32-bit entries" << endl;
		PlotUtil::plotAverageInsertTimePerDimension("./benchmark_Java-extract_1M_3D_32bit.dat", 32);
	} else {
		cerr << "Missing command line argument!" << endl << "valid: 'debug', 'plot', 'rand', 'benchmark'";
		return 1;
	}
};
