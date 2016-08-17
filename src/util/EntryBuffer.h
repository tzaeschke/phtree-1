/*
 * EntryBuffer.h
 *
 *  Created on: Jun 30, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_ENTRYBUFFER_H_
#define SRC_UTIL_ENTRYBUFFER_H_

#include <set>
#include "Entry.h"
#include "util/TEntryBuffer.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <boost/thread/barrier.hpp>

template <unsigned int DIM>
class Node;
template <unsigned int DIM, unsigned int WIDTH>
class PHTree;
template <unsigned int DIM, unsigned int WIDTH>
class EntryBufferPool;

template <unsigned int DIM, unsigned int WIDTH>
class EntryBuffer : public TEntryBuffer<DIM> {
	friend class EntryBufferPool<DIM, WIDTH>;
public:
	EntryBuffer();
	~EntryBuffer() {};

	bool insert(const Entry<DIM, WIDTH>& entry, size_t threadId);
	bool full() const;
	bool empty() const;
	size_t capacity() const;
	void clear();
	Entry<DIM, WIDTH>* init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress, size_t threadId);
	void updateNode(Node<DIM>* node);
	std::pair<Node<DIM>*, unsigned long> getNodeAndAddress();
	Node<DIM>* flushToSubtree();
	void flushToSubtreeSequential();
	bool joinFlushToSubtree();
	void releaseJoinedThreads();
	EntryBufferPool<DIM, WIDTH>* getPool();

	bool assertCleared() const;

private:

	static const size_t CAPACITY = 50;
	static const size_t SEQUENTIAL_FLUSH_IF_ALONE_AFTER = 5;

	// TODO reorder attributes!

	// --------------------
	// needed for insertion
	// --------------------
	bool aloneInInsertion;
	atomic<size_t> nextIndex_;
	size_t suffixBits_;
	size_t initiatorThreadId;
	// - symmetric matrix: need to store n/2 (n+1) fields only
	// - stores the diagonal too for easier access
	// - stores the lower matrix because LCP values of one row are stored together
	unsigned int lcps_[CAPACITY * (CAPACITY + 1) / 2];
	Entry<DIM, WIDTH> buffer_[CAPACITY];
	bool copyCompleted_[CAPACITY];
	bool insertCompleted_[CAPACITY]; // TODO can be merged with copyCompleted

	// ---------------------
	//  needed for flushing
	// ---------------------
	std::mutex m;
	std::condition_variable nextRound;
	std::condition_variable joinFlush;
	std::atomic<size_t> globalThreads_;
	std::atomic<size_t> maxRowMax_;
	size_t activeThreads_;
	size_t readyThreads_;
	size_t nRows_;
	bool lastFlushIteration;
	bool evenRound;
	bool flushing_;
	bool recalculateRange;
	EntryBufferPool<DIM, WIDTH>* pool_;
	bool rowResponsibleForInsert[CAPACITY];
	bool rowEmpty[CAPACITY];
	unsigned int rowMax[CAPACITY];
	unsigned int rowNextMax[CAPACITY];
	unsigned int rowNSuffixes[CAPACITY];
	unsigned int rowNSubnodes[CAPACITY];
	Node<DIM>* rowNodeOdd[CAPACITY];
	Node<DIM>* rowNodeEven[CAPACITY];

	// TODO validation only:
	const Entry<DIM, WIDTH>* originals_[CAPACITY];

	inline void setLcp(unsigned int row, unsigned int column, unsigned int lcp);
	inline unsigned int getLcp(unsigned int row, unsigned int column) const;
	inline void setRowNode(Node<DIM>* node, unsigned int row);
	inline Node<DIM>* getRowNode(unsigned int row);

	void setPool(EntryBufferPool<DIM, WIDTH>* pool);
	bool parallelPoolFlush(bool isCoordinator);
	void joinedThreadDone();
	bool parallelPoolFlushIteration_prepare(size_t startRow, size_t endRow);
	void parallelPoolFlushIteration_print(size_t nThreads) const;
	void parallelPoolFlushIteration_createNodes(size_t startRow, size_t endRow);
	void parallelPoolFlushIteration_insert(size_t startRow, size_t endRow);

	inline void insert(Node<DIM>* currentNode, size_t column, size_t index, size_t suffixBits, bool lock);
};

#include <assert.h>
#include <thread>
#include "util/MultiDimBitset.h"
#include "util/NodeTypeUtil.h"
#include "nodes/Node.h"
#include "PHTree.h"
#include "util/EntryBufferPool.h"

template <unsigned int DIM, unsigned int WIDTH>
EntryBuffer<DIM, WIDTH>::EntryBuffer() : flushing_(false), nextIndex_(0),
	suffixBits_(0), pool_(NULL), lcps_(), buffer_(), insertCompleted_(),
	copyCompleted_() {
}

template <unsigned int DIM, unsigned int WIDTH>
Entry<DIM, WIDTH>* EntryBuffer<DIM, WIDTH>::init(size_t suffixLength, Node<DIM>* node, unsigned long hcAddress, size_t threadId) {
	assert (0 < suffixLength && suffixLength <= (WIDTH - 1));
	assert (!flushing_);
	suffixBits_ = suffixLength * DIM;
	this->node_ = node;
	this->nodeHcAddress = hcAddress;
	nextIndex_ = 1;
	flushing_ = false;
	aloneInInsertion = true;
	initiatorThreadId = threadId;
	assert (!insertCompleted_[0]);
	copyCompleted_[0] = true;
	insertCompleted_[0] = true;
	return &(buffer_[0]);
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::clear() {
	assert (suffixBits_ > 0);

	const size_t i = nextIndex_;
	const size_t c = CAPACITY;
	const size_t n = c; // TODO less work possible: min(i, c);
	const size_t blocksPerEntry = 1 + (suffixBits_ - 1) / MultiDimBitset<DIM>::bitsPerBlock;
	for (unsigned row = 0; row < n; ++row) {
		insertCompleted_[row] = false;
		copyCompleted_[row] = false;

		for (unsigned column = 0; column <= row; ++column) {
			setLcp(row, column, 0);
		}

		for (unsigned block = 0; block < blocksPerEntry; ++block) {
			buffer_[row].values_[block] = 0;
		}
	}

	suffixBits_ = 0;
	nextIndex_ = 0;
	this->node_ = NULL;
	flushing_ = false;
	globalThreads_ = 0;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::assertCleared() const {
	assert (suffixBits_ == 0);
	assert (nextIndex_ == 0);
	assert (!flushing_);
	for (unsigned i = 0; i < CAPACITY; ++i) {
		assert (!insertCompleted_[i]);
		assert (!copyCompleted_[i]);

		for (unsigned j = 0; j < CAPACITY; j++) {
			assert (getLcp(i, j) == 0);
		}
	}

	assert (!this->node_);

	return true;
}

template <unsigned int DIM, unsigned int WIDTH>
inline void EntryBuffer<DIM, WIDTH>::setLcp(unsigned int row, unsigned int column, unsigned int lcp) {
	assert (row < CAPACITY && column < CAPACITY);
	// only stores lower half of the symmetrical matrix
	const unsigned int i = (row > column)? row : column;
	const unsigned int j = (row > column)? column : row;
	const unsigned int index = i * (i + 1) / 2 + j;
	lcps_[index] = lcp;
}

template <unsigned int DIM, unsigned int WIDTH>
inline unsigned int EntryBuffer<DIM, WIDTH>::getLcp(unsigned int row, unsigned int column) const {
	assert (row < CAPACITY && column < CAPACITY);
	// only stores lower half of the symmetrical matrix
	const unsigned int i = (row > column)? row : column;
	const unsigned int j = (row > column)? column : row;
	const unsigned int index = i * (i + 1) / 2 + j;
	return lcps_[index];
}

template <unsigned int DIM, unsigned int WIDTH>
inline void EntryBuffer<DIM, WIDTH>::setRowNode(Node<DIM>* node, unsigned int row) {
	// always set the value to the next round
	if (!evenRound) {
		rowNodeEven[row] = node;
	} else {
		rowNodeOdd[row] = node;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
inline Node<DIM>* EntryBuffer<DIM, WIDTH>::getRowNode(unsigned int row) {
	// always get the value from the current round
	if (evenRound) {
		return rowNodeEven[row];
	} else {
		return rowNodeOdd[row];
	}
}

template <unsigned int DIM, unsigned int WIDTH>
EntryBufferPool<DIM,WIDTH>* EntryBuffer<DIM, WIDTH>::getPool() {
	return pool_;
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::setPool(EntryBufferPool<DIM,WIDTH>* pool) {
	pool_ = pool;
}

template <unsigned int DIM, unsigned int WIDTH>
size_t EntryBuffer<DIM, WIDTH>::capacity() const {
	return CAPACITY;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::full() const {
	return nextIndex_ >= CAPACITY;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::empty() const {
	return nextIndex_ == 0;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::insert(const Entry<DIM, WIDTH>& entry, size_t threadId) {
	assert (suffixBits_ > 0 && suffixBits_ % DIM == 0);
	assert (!flushing_);
	const size_t i = nextIndex_++;
	if (i >= CAPACITY) { return false; }
	// exclusively responsible for index i
	if (threadId != initiatorThreadId) { aloneInInsertion = false; };

	assert (0 < i && i < CAPACITY);
	assert (!insertCompleted_[i]);
	// copy ID and necessary bits into the local buffer
	MultiDimBitset<DIM>::duplicateLowestBitsAligned(entry.values_, suffixBits_, buffer_[i].values_);
	copyCompleted_[i] = true;
	buffer_[i].id_ = entry.id_;
	originals_[i] = &entry;

	// compare the new entry to all previously inserted entries
	const unsigned int startIndexDim = WIDTH - (suffixBits_ / DIM);
	for (unsigned other = 0; other < i; ++other) {
		while (!copyCompleted_[other]) {}; // spin until the previous thread is done
		// TODO no need to compare all values!
		// TODO no need to compare to full values!
		assert (getLcp(other, i) == 0 && getLcp(i, other) == 0);
		assert (MultiDimBitset<DIM>::checkRangeUnset(buffer_[other].values_, DIM * WIDTH, suffixBits_));
		const pair<bool, size_t> comp = MultiDimBitset<DIM>::compare(
				entry.values_, DIM * WIDTH, startIndexDim, WIDTH, buffer_[other].values_, suffixBits_);
		assert(!comp.first);
//		assert (!flushing_);
		setLcp(i, other, comp.second);
	}

	insertCompleted_[i] = true;
	return true;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::joinFlushToSubtree() {
	return parallelPoolFlush(false);
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::releaseJoinedThreads() {
	// wait (spin) until all other threads have unregistered
	while (activeThreads_ > 1) {}
	unique_lock<mutex> lock(m);
	assert (activeThreads_ == 1);
	assert (!flushing_);
	joinFlush.notify_all();
}

template<unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::flushToSubtreeSequential() {
	assert(suffixBits_ > 0);
	assert((this->node_->lookup(this->nodeHcAddress, true).exists) && (this->node_->lookup(this->nodeHcAddress, true).hasSpecialPointer));

	assert(!flushing_);
	flushing_ = true;
	const size_t currentIndex = nextIndex_;
	const size_t capacity = CAPACITY;
	nRows_ = min(currentIndex, capacity);
	maxRowMax_ = -1uL;
	assert(nRows_ > 0 && nRows_ <= CAPACITY);

	for (unsigned row = 0; row < n; ++row) {
		assert (insertCompleted_[row]);
		rowEmpty[row] = false;
		rowNode[row] = NULL;
		rowMax[row] = -1u; // TODO not needed ?!
		rowNextMax[row] = -1u;
	}

	while (parallelPoolFlushIteration_prepare(0, nRows_)) {
#ifdef PRINT
		parallelPoolFlushIteration_print(1);
#endif
		parallelPoolFlushIteration_createNodes(0, nRows_);
		parallelPoolFlushIteration_insert(0, nRows_);
	}

	// insert the root node of the subtree into the parent
	this->node_->insertAtAddress(this->nodeHcAddress, rowNode[0]);
}

template <unsigned int DIM, unsigned int WIDTH>
Node<DIM>* EntryBuffer<DIM, WIDTH>::flushToSubtree() {
	assert (suffixBits_ > 0);
	assert ((this->node_->lookup(this->nodeHcAddress, true).exists)
				&& (this->node_->lookup(this->nodeHcAddress, true).hasSpecialPointer));

	unique_lock<mutex> lock(m);
	assert (!flushing_);
	flushing_ = true;
	evenRound = true;
	lastFlushIteration = false;
	activeThreads_ = 0;
	lock.unlock();

	const size_t currentIndex = nextIndex_;
	const size_t capacity = CAPACITY;
	nRows_ = min(currentIndex, capacity);
	assert (nRows_ == capacity);
	maxRowMax_ = -1uL;
	for (unsigned row = 0; row < nRows_; ++row) {
		// spin until remaining insertions are done
		while (!insertCompleted_[row]) {}
		rowEmpty[row] = false;
		rowNodeEven[row] = NULL;
		rowNodeOdd[row] = NULL;
		rowMax[row] = -1u; // TODO not needed ?!
		rowNextMax[row] = -1u;
	}

	parallelPoolFlush(true);

	// flushing is done so insert the root of the subtree into the parent
	Node<DIM>* subtreeRoot = getRowNode(0);
	assert ((this->node_->lookup(this->nodeHcAddress, true).exists)
			&& (this->node_->lookup(this->nodeHcAddress, true).hasSpecialPointer));
	this->node_->insertAtAddress(this->nodeHcAddress, subtreeRoot);

#ifndef NDEBUG
	// Validate all copied entries (except for the first one which was only copied partially)
	for (unsigned i = 0; i < nRows_; ++i) {
		assert (insertCompleted_[i]);
		assert (i == 0 || rowEmpty[i]);
	}
#endif

	releaseJoinedThreads();
	return subtreeRoot;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::parallelPoolFlush(bool isCoordinator) {

	unique_lock<mutex> lock(m);
	if (!flushing_) return false;

	const size_t threadIndex = globalThreads_++;
	if (isCoordinator) {
		lastFlushIteration = false;
		recalculateRange = true;
		lock.unlock();
	} else {
		joinFlush.wait(lock);
		lock.unlock();
		if (!flushing_) {return true;}
	}

	size_t startRow, endRow;
	while (true) {
		assert ((readyThreads_ <= activeThreads_) && (activeThreads_ <= globalThreads_));

		// prepare for new iteration
		if (isCoordinator) {
			// check if new threads want to join the flush
			if ((activeThreads_ != globalThreads_)) {
				lock.lock();
				activeThreads_ = globalThreads_;
				recalculateRange = true;
				joinFlush.notify_all();
				lock.unlock();
			}

			// spin until all threads are ready for the next iteration
			while (readyThreads_ < activeThreads_) { }
			evenRound = !evenRound;
			if (lastFlushIteration) {flushing_ = false;}
			lastFlushIteration = !lastFlushIteration;
			nextRound.notify_all();
		} else {
			lock.lock();
			readyThreads_ += 1;
			nextRound.wait(lock);
			lock.unlock();
		}

		if (!flushing_) { break; }

		// distribute the work on the matrix equally over all participating threads
		if (recalculateRange) {
			const size_t rowsPerThread = nRows_ / activeThreads_ + 1;
			startRow = threadIndex * rowsPerThread;
			endRow = min((threadIndex + 1) * rowsPerThread, nRows_);
		}

		// execute all flushing steps per iteration in parallel and globally mark if there is more work
		if (parallelPoolFlushIteration_prepare(startRow, endRow)) {
			parallelPoolFlushIteration_createNodes(startRow, endRow);
			parallelPoolFlushIteration_insert(startRow, endRow);
			lastFlushIteration = false;
		}
	}

	if (!isCoordinator) {
		lock.lock();
		// count down threads
		activeThreads_ -= 1;
		joinFlush.wait(lock);
		lock.unlock();
	}

	return true;
}

template <unsigned int DIM, unsigned int WIDTH>
bool EntryBuffer<DIM, WIDTH>::parallelPoolFlushIteration_prepare(size_t startRow, size_t endRow) {

	size_t globalMaxRowMax = maxRowMax_;
	size_t localMaxRowMax = -1uL;
	bool hasMoreRows = false;
	// calculate maximum and number of occurrences per row
	// TODO no need to revisit rows that were not changed!
	for (unsigned row = startRow; row < endRow; ++row) {
		if (rowEmpty[row]) continue;
		hasMoreRows = row != 0;
		bool responsibleForRow = true;

//		if (lastMaxRowMax == rowMax[row]) {
		// the row needs to be updated as the maximum changed

		rowMax[row] = -1u; // TODO possible similar to: (rowNextMax[row] == (-1u))? -1u : rowNextMax[row] - 1;
		rowNextMax[row] = -1u;

		for (unsigned column = 0; column < nRows_; ++column) { // TODO how to save iterations?
			if (rowEmpty[column] || row == column) continue;

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
				rowResponsibleForInsert[row] = column > row;
				rowNextMax[row] = rowMax[row];
				rowMax[row] = currentLcp;
				rowNSuffixes[row] = 0;
				rowNSubnodes[row] = 0;
				if (getRowNode(row))	{++rowNSubnodes[row];}
				else 					{++rowNSuffixes[row];}
				if (getRowNode(column)) {++rowNSubnodes[row];}
				else 					{++rowNSuffixes[row];}
			} else { // TODO probably more likely and less expensive than case above
				assert ((1 + rowNextMax[row]) < (1 + currentLcp));
				// the second highest value needs to be updated
				rowNextMax[row] = currentLcp;
			}

			assert (rowNextMax[row] < rowMax[row] || rowNextMax[row] == (-1u));
		}
	//			}

		rowResponsibleForInsert[row] = responsibleForRow;
		assert ((1 + rowNextMax[row]) <= (1 + rowMax[row]));
		//			assert (rowNSuffixes[row] + rowNSubnodes[row] <= (1uL << DIM));
		if ((1 + rowMax[row]) > (1 + localMaxRowMax)) {localMaxRowMax = rowMax[row];}
	}

	// set the maximum value of the matrix globally
	if (hasMoreRows) {
		while (!atomic_compare_exchange_strong(&maxRowMax_, &globalMaxRowMax, localMaxRowMax)) {
			// the global value was changed so break if it was bigger than the local one
			if (globalMaxRowMax > localMaxRowMax) break;
		}
	}

	return hasMoreRows;
}

template <unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::parallelPoolFlushIteration_print(size_t nThreads) const {
	cout << nThreads << " thread(s) working on flush" << endl;
	cout << "type\tmax\tnext\t#suff\t#node\t| i";
	for (unsigned row = 0; row < nRows_; ++row) { cout << "\t"  << row;}
	cout << endl;
	// print the current LCP matrix
	for (unsigned int row = 0; row < nRows_; ++row) {
		if (getRowNode(row)) { cout << "(node)\t"; } else { cout << "(suff)\t"; }
		if (rowMax[row] == -1u) { cout << "-1\t"; } else { cout << rowMax[row] << "\t"; }
		if (rowNextMax[row] == -1u) { cout << "-1\t"; } else {cout << rowNextMax[row] << "\t"; }
		cout << rowNSuffixes[row] << "\t" << rowNSubnodes[row] << "\t| " << row;
		for (unsigned col = 0; col < nRows_; ++col) {
			cout << "\t";
			if (rowEmpty[row] || rowEmpty[col]) {cout << "-";}
			else if (getLcp(row, col) == (-1u)) {cout << "(-1)";}
			else {
				cout << getLcp(row, col);
				if (getLcp(row, col) == maxRowMax_) {cout << "*";}
			}
		}
		cout << endl;
	}
	cout << endl;
}

template<unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::parallelPoolFlushIteration_createNodes(size_t startRow, size_t endRow) {

	// current suffix bits: buffer suffix bits - current longest prefix bits - HC address bits (one node)
	const unsigned int suffixBits = suffixBits_ - DIM * (maxRowMax_ + 1);
	const unsigned int basicMsbIndex = WIDTH - suffixBits_ / DIM;
	const unsigned int index = basicMsbIndex + maxRowMax;

	// create a new node for each row that was not ruled out yet and reinsert its current contents (either subnode or suffix)
	for (unsigned row = startRow; row < endRow; ++row) {
		if (rowEmpty[row] || maxRowMax != rowMax[row] || !rowResponsibleForInsert[row])
			continue;

		assert((1 + rowMax[row]) > (1 + rowNextMax[row]));
		// <nextMax>
		// <       current max        >
		// [ ------ |-  prefix -| DIM | .... ]
		const unsigned int prefixBits = (rowMax[row] - (1 + rowNextMax[row])) * DIM;
		const unsigned int nEntries = rowNSuffixes[row] + rowNSubnodes[row];
		assert(nEntries <= (1uL << DIM));
		Node<DIM>* currentNode = NodeTypeUtil<DIM>:: template buildNodeWithSuffixes<WIDTH>(prefixBits,
				nEntries, rowNSuffixes[row], suffixBits);
		if (prefixBits > 0) {
			const unsigned int currentBits = suffixBits + DIM + prefixBits;
			assert(currentBits <= suffixBits_);
			MultiDimBitset<DIM>::duplicateBits(buffer_[row].values_, currentBits, prefixBits / DIM,
					currentNode->getPrefixStartBlock());
		}

		// insert the previous value of the row into the new node
		insert(currentNode, row, index, suffixBits, false);

		setRowNode(currentNode, row);
	}
}

template<unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::insert(Node<DIM>* currentNode, size_t column, size_t index, size_t suffixBits, bool lock) {

	const unsigned long hcAddress = MultiDimBitset<DIM>::interleaveBits(buffer_[column].values_, index, DIM * WIDTH);
	assert(!(currentNode->lookup(hcAddress, true).exists));
	assert(currentNode->getNumberOfContents() < currentNode->getMaximumNumberOfContents());

	boost::unique_lock<boost::shared_mutex> lk(rowNode[column]->rwLock, boost::defer_lock);
	if (lock) { lk.lock(); }

	if (rowNode[column]) {
		// insert the subnode
		currentNode->insertAtAddress(hcAddress, rowNode[column]);
		// TODO double???					setLcp(row, column, rowNextMax[row]);
	} else {
		// insert the suffix
		// TODO maybe sort before insertion so there is no need for swapping and sorting and shifting in LHC
		if (currentNode->canStoreSuffixInternally(suffixBits)) {
			// store internally
			unsigned long suffix = 0uL;
			MultiDimBitset<DIM>::removeHighestBits(buffer_[column].values_, DIM * WIDTH, index + 1, &suffix);
			currentNode->insertAtAddress(hcAddress, suffix, buffer_[column].id_);
		} else {
			// store externally
			assert(currentNode->canStoreSuffix(suffixBits) == 0);
			const pair<unsigned long*, unsigned int> suffixStartBlock = currentNode->reserveSuffixSpace(suffixBits);
			currentNode->insertAtAddress(hcAddress, suffixStartBlock.second,buffer_[column].id_);
			MultiDimBitset<DIM>::duplicateLowestBitsAligned(buffer_[column].values_, suffixBits,suffixStartBlock.first);
			assert(currentNode->lookup(hcAddress, true).suffixStartBlock == suffixStartBlock.first);
		}
	}
}

template<unsigned int DIM, unsigned int WIDTH>
void EntryBuffer<DIM, WIDTH>::parallelPoolFlushIteration_insert(size_t startRow, size_t endRow) {
	const size_t maxRowMax = maxRowMax_;
	const unsigned int basicMsbIndex = WIDTH - suffixBits_ / DIM;

	// create a new node for each row that was not ruled out yet
	const unsigned int index = basicMsbIndex + maxRowMax;
	assert(index < WIDTH);
	// current suffix bits: buffer suffix bits - current longest prefix bits - HC address bits (one node)
	const unsigned int suffixBits = suffixBits_ - DIM * (maxRowMax + 1);
	for (unsigned row = startRow; row < endRow; ++row) {
		assert (rowEmpty[row] || maxRowMax >= rowMax[row]);
		if (rowEmpty[row] || maxRowMax != rowMax[row])
			continue;

		// a thread is responsible for handling a maximum value in row x
		// if there is no previous row that is already handling a row including column x
		// TODO this is not needed if there is only 1 thread!
		bool responsible = true;
		for (unsigned column = 0; multiThread && column < row && responsible; ++column) {
			responsible = getLcp(row, column) != maxRowMax;
		}

		// insert all entries within this row into the new sub node
		for (unsigned column = row + 1; column < nRows_ && responsible; ++column) {
			const unsigned int currentLcp = getLcp(row, column);
			assert (rowEmpty[column] || (currentLcp <= maxRowMax && currentLcp <= rowMax[column] && currentLcp <= rowMax[row]));
			if (rowEmpty[column] || currentLcp != rowMax[row])
				continue;

			insert(rowNode[row], column, index, suffixBits, multiThread);
			assert (!rowEmpty[column]);
			rowEmpty[column] = true;
			rowNode[column] = rowNode[row];
		}

		setLcp(row, row, rowNextMax[row]); // TODO not needed?!
	}
}


#endif /* SRC_UTIL_ENTRYBUFFER_H_ */
