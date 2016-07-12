/*
 * EntryBuffer.h
 *
 *  Created on: Jun 30, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_ENTRYBUFFER_H_
#define SRC_UTIL_ENTRYBUFFER_H_

#include <assert.h>
#include <set>
#include "Entry.h"
#include "util/TEntryBuffer.h"

template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class PHTree;

template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer : public TEntryBuffer<DIM> {
public:
	EntryBuffer();
	~EntryBuffer() {};

	bool insert(const Entry<DIM, WIDTH>& entry);
	bool full() const;
	bool empty() const;
	size_t size() const;
	size_t capacity() const;
	void clear();
	Entry<DIM, WIDTH>* init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress);
	void updateNode(Node<DIM>* node);
	std::pair<Node<DIM>*, unsigned long> getNodeAndAddress();
	Node<DIM>* flushToSubtree(PHTree<DIM, WIDTH>& tree);

private:
	static const size_t capacity_ = 10;

	size_t nextIndex_;
	size_t suffixBits_;
	// TODO symmetric matrix with no information on diagonal: need to store n/2 (n-1) fields only
	unsigned int lcps_[capacity_ * capacity_];
	Entry<DIM, WIDTH> buffer_[capacity_];

	// TODO validation only:
	const Entry<DIM, WIDTH>* originals_[capacity_];

	inline void setLcp(unsigned int row, unsigned int column, unsigned int lcp);
	inline unsigned int getLcp(unsigned int row, unsigned int column) const;
};

#include "util/MultiDimBitset.h"
#include "util/NodeTypeUtil.h"
#include "nodes/Node.h"
#include "PHTree.h"

template <unsigned int DIM, unsigned int WIDTH>
EntryBuffer<DIM, WIDTH>::EntryBuffer() : nextIndex_(0), suffixBits_(0), lcps_(), buffer_() {
}

template <unsigned int DIM, unsigned int WIDTH>
Entry<DIM, WIDTH>* EntryBuffer<DIM, WIDTH>::init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress) {
	clear();
	assert (suffixLength <= (WIDTH - 1));
	suffixBits_ = suffixLength * DIM;
	this->node_ = node;
	this->nodeHcAddress = hcAddress;
	nextIndex_ = 1;
	return &(buffer_[0]);
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::clear() {
	nextIndex_ = 0;
	suffixBits_ = 0;
	for (unsigned row = 0; row < capacity_; ++row) {
		for (unsigned column = 0; column < capacity_; ++column) {
			setLcp(row, column, 0);
		}
	}
}

template <unsigned int DIM, unsigned int WIDTH>
inline void EntryBuffer<DIM, WIDTH>::setLcp(unsigned int row, unsigned int column, unsigned int lcp) {
	assert (row < capacity_ && column < capacity_);
	const unsigned int index = row * capacity_ + column;
	lcps_[index] = lcp;
}

template <unsigned int DIM, unsigned int WIDTH>
inline unsigned int EntryBuffer<DIM, WIDTH>::getLcp(unsigned int row, unsigned int column) const {
	assert (row < capacity_ && column < capacity_);
	const unsigned int index = row * capacity_ + column;
	return lcps_[index];
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::size() const {
	return nextIndex_;
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::capacity() const {
	return capacity_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::full() const {
	assert (nextIndex_ <= capacity_);
	return nextIndex_ == capacity_;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::empty() const {
	return nextIndex_ == 0;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>& entry) {
	assert (!full());
	assert (suffixBits_ > 0 && suffixBits_ % DIM == 0);

	// copy ID and necessary bits into the local buffer
	buffer_[nextIndex_].id_ = entry.id_;
	MultiDimBitset<DIM>::duplicateLowestBitsAligned(entry.values_, suffixBits_, buffer_[nextIndex_].values_);

	originals_[nextIndex_] = &entry;

	// compare the new entry to all previously inserted entries
	const unsigned int startIndexDim = WIDTH - (suffixBits_ / DIM);
	for (unsigned other = 0; other < nextIndex_; ++other) {
		// TODO no need to compare all values!
		// TODO no need to compare to full values!
		assert (getLcp(other, nextIndex_) == 0 && getLcp(nextIndex_, other) == 0);
		assert (MultiDimBitset<DIM>::checkRangeUnset(buffer_[other].values_, DIM * WIDTH, suffixBits_));
		const pair<bool, size_t> comp = MultiDimBitset<DIM>::compare(
				entry.values_, DIM * WIDTH, startIndexDim, WIDTH, buffer_[other].values_, suffixBits_);
		assert(!comp.first);
		setLcp(other, nextIndex_, comp.second); // TODO only one set necessary with half matrix!
		setLcp(nextIndex_, other, comp.second);
	}

	++nextIndex_;

	// empty the buffer if it is full
	return full();
}


template <unsigned int DIM, unsigned int WIDTH>
Node<DIM>* EntryBuffer<DIM, WIDTH>::flushToSubtree(PHTree<DIM, WIDTH>& tree) {
	assert (nextIndex_ > 0 && nextIndex_ <= capacity_);

	// builds the subtree from bottom up
	bool rowEmpty[capacity_];
	unsigned int rowMax[capacity_];
	unsigned int rowNextMax[capacity_];
	unsigned int rowNSuffixes[capacity_];
	unsigned int rowNSubnodes[capacity_];
	unsigned int rowNextValidOffset[capacity_];
	Node<DIM>* rowNode[capacity_];
	for (unsigned row = 0; row < nextIndex_; ++row) {
		rowEmpty[row] = false;
		rowNextValidOffset[row] = 1;
		rowNode[row] = NULL;
		rowMax[row] = -1u; // TODO not needed ?!
		rowNextMax[row] = -1u;
	}

	const unsigned int basicMsbIndex = WIDTH - suffixBits_ / DIM;
	bool hasMoreRows = true;
	while (hasMoreRows) {
		hasMoreRows = false;
		unsigned int maxRowMax = 0;

		// calculate maximum and number of occurrences per row
		// TODO no need to revisit rows that were not changed!
		for (unsigned row = 0; row < nextIndex_; row += rowNextValidOffset[row]) {
			if (rowEmpty[row]) {continue;}
			assert (rowNextValidOffset[row] != 0);
			hasMoreRows = row != 0;

//			if (lastMaxRowMax == rowMax[row]) {
				// the row needs to be updated as the maximum changed

				rowMax[row] = -1u; // TODO possible similar to: (rowNextMax[row] == (-1u))? -1u : rowNextMax[row] - 1;
				rowNextMax[row] = -1u;

				for (unsigned column = 0; column < nextIndex_; column += rowNextValidOffset[column]) { // TODO how to save iterations?
					if (rowEmpty[column] || row == column) {continue;}

					const unsigned int currentLcp = getLcp(row, column);
					if ((1 + currentLcp) <= (1 + rowNextMax[row])) {
						// most common case: no need to update any values
						continue;
					} else if (currentLcp == rowMax[row] && rowNode[column]) {
						// found another LCP of the same length that has a subnode
						++rowNSubnodes[row];
					} else if (currentLcp == rowMax[row]) {
						// found another LCP of the same length without a subnode
						++rowNSuffixes[row];
					} else if ((1 + currentLcp) > (1 + rowMax[row])) {
						// found a higher LCP: store the new one
						rowNextMax[row] = rowMax[row];
						rowMax[row] = currentLcp;
						rowNSuffixes[row] = 0;
						rowNSubnodes[row] = 0;
						if (rowNode[row])	{++rowNSubnodes[row];}
						else 				{++rowNSuffixes[row];}
						if (rowNode[column]){++rowNSubnodes[row];}
						else 				{++rowNSuffixes[row];}
					} else { // TODO probably more likely and less expensive than case above
						assert ((1 + rowNextMax[row]) < (1 + currentLcp));
						// the second highest value needs to be updated
						rowNextMax[row] = currentLcp;
					}

					assert (rowNextMax[row] < rowMax[row] || rowNextMax[row] == (-1u));
				}
//			}

			assert ((1 + rowNextMax[row]) <= (1 + rowMax[row]));
//			assert (rowNSuffixes[row] + rowNSubnodes[row] <= (1uL << DIM));
			if ((1 + rowMax[row]) > (1 + maxRowMax)) {maxRowMax = rowMax[row];}
		}

#ifdef PRINT
		// print the current LCP matrix
		cout << "type\tmax\tnext\t#suff\t#node\t| i";
		for (unsigned row = 0; row < nextIndex_; ++row) { cout << "\t"  << row;}
		cout << endl;

		for (unsigned row = 0; row < nextIndex_; ++row) {
			if (rowNode[row]) { cout << "(node)\t"; }
			else { cout << "(suff)\t"; }
			if (rowMax[row] == -1u) { cout << "-1\t"; }
			else { cout << rowMax[row] << "\t"; }
			if (rowNextMax[row] == -1u) { cout << "-1\t"; }
			else {cout << rowNextMax[row] << "\t"; }
			cout << rowNSuffixes[row] << "\t";
			cout << rowNSubnodes[row] << "\t| ";

			cout << row;

			for (unsigned col = 0; col < nextIndex_; ++col) {
				cout << "\t";
				if (rowEmpty[row] || rowEmpty[col]) {cout << "x (+" << rowNextValidOffset[col] << ")";}
				else if (getLcp(row, col) == (-1u)) {cout << "(-1)";}
				else {
					cout << getLcp(row, col);
					if (getLcp(row, col) == maxRowMax) {cout << "*";}
				}
			}
			cout << endl;
		}
		cout << endl;
#endif

		// create a new node for each row that was not ruled out yet
		const unsigned int index = basicMsbIndex + maxRowMax;
		// current suffix bits: buffer suffix bits - current longest prefix bits - HC address bits (one node)
		const unsigned int suffixBits = suffixBits_ - DIM * (maxRowMax + 1);
		for (unsigned row = 0; row < nextIndex_ && hasMoreRows; row += rowNextValidOffset[row]) {
			if (rowEmpty[row] || maxRowMax != rowMax[row]) {continue;}

			setLcp(row, row, maxRowMax);
			assert ((1 + rowMax[row]) > (1 + rowNextMax[row]));
			// <nextMax>
			// <       current max        >
			// [ ------ |-  prefix -| DIM | .... ]
			const unsigned int prefixBits = (rowMax[row] - (1 + rowNextMax[row])) * DIM;
			const unsigned int nEntries = rowNSuffixes[row] + rowNSubnodes[row];
			assert (nEntries <= (1uL << DIM));
			Node<DIM>* currentNode = NodeTypeUtil<DIM>::
					template buildNodeWithSuffixes<WIDTH>(prefixBits, nEntries, rowNSuffixes[row], suffixBits);
			if (prefixBits > 0) {
				const unsigned int currentBits = suffixBits + DIM + prefixBits;
				assert (currentBits <= suffixBits_);
				MultiDimBitset<DIM>::duplicateBits(buffer_[row].values_,
						currentBits, prefixBits / DIM, currentNode->getPrefixStartBlock());
			}

			// insert all entries within this row into the new sub node
			for (unsigned column = row; column < nextIndex_; column += rowNextValidOffset[column]) {
				const unsigned int currentLcp = getLcp(row, column);
				 if (rowEmpty[column] || currentLcp != rowMax[row]) {continue;} // TODO != maxRowMax?!

				const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(buffer_[column].values_,index , DIM * WIDTH);
				assert (!(currentNode->lookup(hcAddress, true).exists));
				assert (currentNode->getNumberOfContents() < currentNode->getMaximumNumberOfContents());
				if (rowNode[column]) {
					// insert the subnode
					currentNode->insertAtAddress(hcAddress, rowNode[column]);
					 // TODO double???					setLcp(row, column, rowNextMax[row]);
				} else {
					// insert the suffix
					// TODO maybe sort before insertion so there is no need for swapping and sorting and shifting in LHC

					if (currentNode->canStoreSuffixInternally(suffixBits)) {
						unsigned long suffix = 0uL;
						MultiDimBitset<DIM>::removeHighestBits(buffer_[column].values_, DIM * WIDTH, index + 1, &suffix);
						currentNode->insertAtAddress(hcAddress, suffix, buffer_[column].id_);
					} else {
						assert (currentNode->canStoreSuffix(suffixBits) == 0);
						const pair<unsigned long*, unsigned int> suffixStartBlock = currentNode->reserveSuffixSpace(suffixBits);
						currentNode->insertAtAddress(hcAddress, suffixStartBlock.second, buffer_[column].id_);
						MultiDimBitset<DIM>::duplicateLowestBitsAligned(buffer_[column].values_, suffixBits, suffixStartBlock.first);
						assert(currentNode->lookup(hcAddress, true).suffixStartBlock == suffixStartBlock.first);
					}
				}

				rowEmpty[column] |= (row != column);
				rowNode[column] = currentNode;

				assert (rowNextValidOffset[column] == 1 && !rowEmpty[0]);
				if (row != column) {
					// invalidate the current entry and skip it in future runs
					unsigned int firstEmptyCol;
					for (firstEmptyCol = column; rowEmpty[firstEmptyCol]; --firstEmptyCol);
					rowNextValidOffset[firstEmptyCol + 1] = column - firstEmptyCol;
					if (column + 1 < nextIndex_ && rowEmpty[column + 1]) {
						//       _ x x x (x) x x x x _
						// prev:   ----->    ------->
						// new:    ----------------->
						rowNextValidOffset[firstEmptyCol + 1] += rowNextValidOffset[column + 1];
					}
				}
			}

			setLcp(row, row, rowNextMax[row]); // TODO not needed?!
		}
	}

	assert ((this->node_->lookup(this->nodeHcAddress, true).exists)
			&& (this->node_->lookup(this->nodeHcAddress, true).hasSpecialPointer));
	this->node_->insertAtAddress(this->nodeHcAddress, rowNode[0]);

#ifndef NDEBUG
	// Validate all entries (except for the first one which was only copied partially)
	for (unsigned i = 1; i < nextIndex_; ++i) {
		assert (rowEmpty[i]);
		assert (tree.lookup(*(originals_[i])).first);
	}
#endif

	return rowNode[0];
}


#endif /* SRC_UTIL_ENTRYBUFFER_H_ */
