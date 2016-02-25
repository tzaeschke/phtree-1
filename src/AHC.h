/*
 * AHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_AHC_H_
#define SRC_AHC_H_

#include "Node.h"
#include <vector>

using namespace std;

class AHC: public Node {
public:
	AHC(int dim, int valueLength);
	virtual ~AHC();

	bool lookup(Entry* e, int depth, int index);

	void insert(Entry* e, int depth, int index);

	vector<vector<bool>> getSuffix(long hcAddress);

	size_t getSuffixSize(long hcAddress);

protected:
	int dim_;
	int valueLength_;
	vector<bool> filled_;
	vector<bool> hasSubnode_;
	vector<Node *> subnodes_;
	// entry -> value -> bit
	vector<vector<vector<bool>>> suffixes_;

	void removeFirstBits(size_t nBitsToRemove, vector<vector<bool>> *values);

	void duplicateFirstBits(size_t nBitsToDuplicate, vector<vector<bool>> from, vector<vector<bool>>* to);
};

#endif /* SRC_AHC_H_ */
