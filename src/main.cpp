#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <algorithm>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"

#define PLOT_DATA_FILE "./plot/data/phtree.dat"

#define BIT_LENGTH 	8
#define ENTRY_DIM 	2
#define ENTRY_DIMS {2, 5, 20};

#define N_REPETITIONS 10
#define N_RANDOM_ENTRIES 100

using namespace std;

int mainSimpleExample()
{
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

int mainPlot() {

	srand(time(NULL));

	// step 1: gather phtree data
	// 1.1. create X random entries for Y different dimensions
	cout << "Creating " << N_RANDOM_ENTRIES	<< " random entries per dimension..." << endl;
	size_t dimTests[] = ENTRY_DIMS;
	size_t dimTestsSize = sizeof(dimTests) / sizeof(*dimTests);
	vector<vector<Entry*>> randomEntries;
	for (size_t test = 0; test < dimTestsSize; test++) {
		vector<Entry*> randomDimEntries;
		for (size_t nEntry = 0; nEntry < N_RANDOM_ENTRIES; nEntry++) {
			vector<int>* entryValues = new vector<int>(dimTests[test]);
			for (size_t dim = 0; dim < dimTests[test]; dim++) {
				entryValues->at(dim) = rand() % (2 << BIT_LENGTH);
			}
			Entry* entry = new Entry(*entryValues, BIT_LENGTH);

			if (find(randomDimEntries.begin(), randomDimEntries.end(), entry)
					!= randomDimEntries.end()) {
				// duplicate
				nEntry--;
			} else {
				randomDimEntries.push_back(entry);
			}
		}
		randomEntries.push_back(randomDimEntries);
	}

	// 1.2. add all entries to the PH-Tree and measure the time per insertion
	cout << "inserting all entries into a PH-Tree while logging the time per insertion..." << endl;


	vector<PHTree*> phtrees;
	for (size_t test = 0; test < dimTestsSize; test++) {
		PHTree* tree = new PHTree(dimTests[test], BIT_LENGTH);
		phtrees.push_back(tree);
	}

	vector<vector<unsigned int>> insertMillis(N_RANDOM_ENTRIES);
	for (size_t iEntry = 0; iEntry < N_RANDOM_ENTRIES; iEntry++) {
		insertMillis.at(iEntry) = vector<unsigned int>(dimTestsSize);
		for (size_t test = 0; test < dimTestsSize; test++) {
			Entry* entry = randomEntries[test][iEntry];
			unsigned int startTime = clock();
			phtrees[test]->insert(entry);
			unsigned int totalMillis = clock() - startTime;
			assert(phtrees[test]->lookup(entry));
			insertMillis.at(iEntry).at(test) = totalMillis;
		}
	}

	// write gathered data into a file
	ofstream plotFile;
	plotFile.open(PLOT_DATA_FILE, ofstream::out | ofstream::trunc);
	int entryIndex = 0;
	for (size_t iEntry = 0; iEntry < N_RANDOM_ENTRIES; iEntry++) {
			plotFile << (entryIndex++);
			for (size_t test = 0; test < dimTestsSize; test++) {
				plotFile << "\t" << insertMillis[iEntry][test];
			}
			plotFile << "\n";
		}
	plotFile.close();

	// step 2: call Gnuplot
	cout << "calling gnuplot..." << endl;
	system("gnuplot -p './plot/phtree.p'");

	return 0;
};

int main(int argc, char* argv[]) {

	for (int i = 0; i < argc; i++) {
		cout << "argv[" << i << "]= " << argv[i] << endl;
	}

	string debug = "debug";
	string plot = "plot";

	if (argc != 2 || debug.compare(argv[1]) == 0) {
		return mainSimpleExample();
	} else if (plot.compare(argv[1]) == 0) {
		return mainPlot();
	} else {
		cerr << "Missing command line argument!" << endl;
		return 1;
	}
};
