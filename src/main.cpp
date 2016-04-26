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

using namespace std;

int mainSimpleExample() {
	const unsigned int bitLength = 8;
	vector<unsigned long> e1Values { 10, 5 };
	vector<unsigned long> e2Values { 11, 12 };
	vector<unsigned long> e3Values { 60, 7 };
	vector<unsigned long> e4Values { 1, 3 };
	vector<unsigned long> e5Values { 11, 5 };
	Entry* e1 = new Entry(e1Values, bitLength, 1);
	Entry* e2 = new Entry(e2Values, bitLength, 2);
	Entry* e3 = new Entry(e3Values, bitLength, 3);
	Entry* e4 = new Entry(e4Values, bitLength, 4);
	Entry* e5 = new Entry(e5Values, bitLength, 5);

	CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
	uint64_t sta = RDTSC();
	PHTree* phtree = new PHTree(2, bitLength);
	phtree->insert(e1);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	cout << *phtree;
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e2);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e1).second == 1);
	assert (!phtree->lookup(e3).first);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e3);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e3).second == 3);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e4);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e2).second == 2);
	assert (phtree->lookup(e4).second == 4);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e5);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e5).second == 5);
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
	delete e5;

	return 0;
}

int main(int argc, char* argv[]) {

	string debug = "debug";
	string plot = "plot";
	string rand = "rand";
	string benchmark = "benchmark";

	#ifndef NDEBUG
		cout << "assertions enabled!" << endl;
	#endif

	if (argc != 2 || debug.compare(argv[1]) == 0) {
		return mainSimpleExample();
	} else if (plot.compare(argv[1]) == 0) {
//		PlotUtil::plotAverageInsertTimePerDimension("./plot/data/phtree_java_rand_unique_entries.dat", 32);
//		PlotUtil::plotTimeSeriesOfInserts();
		PlotUtil::plotAverageInsertTimePerDimensionRandom();
		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom();
		return 0;
	} else if (rand.compare(argv[1]) == 0) {
		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom();
	} else if (benchmark.compare(argv[1]) == 0) {
		cout << "run a benchmark extracted from the Java implementation with 1M 3D 32-bit entries" << endl;
		PlotUtil::plotAverageInsertTimePerDimension("./benchmark_Java-extract_1M_3D_32bit.dat", 32);
//		cout << "run a benchmark extracted from the Java implementation with 1M 6D 64-bit entries" << endl;
//		PlotUtil::plotAverageInsertTimePerDimension("./benchmark_Java-extract_1M_6D_64bit.dat", 64);
//		cout << "run a benchmark extracted from the Java implementation with 1M 10D 96-bit entries" << endl;
//				PlotUtil::plotAverageInsertTimePerDimension("./benchmark_Java-extract_1M_10D_96bit.dat", 96);
	} else {
		cerr << "Missing command line argument!" << endl << "valid: 'debug', 'plot', 'rand', 'benchmark'";
		return 1;
	}
};
