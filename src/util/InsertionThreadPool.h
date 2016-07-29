/*
 * ThreadPool.h
 *
 *  Created on: Jul 18, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_INSERTIONTHREADPOOL_H_
#define SRC_UTIL_INSERTIONTHREADPOOL_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <atomic>
#include <boost/thread/barrier.hpp>

template <unsigned int DIM, unsigned int WIDTH>
class PHTree;
template <unsigned int DIM, unsigned int WIDTH>
class Entry;
template <unsigned int DIM, unsigned int WIDTH>
class EntryBufferPool;


enum InsertionOrder {
	sequential_entries,
	range_per_thread, // DEFAULT
	sequential_ranges
};

enum InsertionApproach {
	optimistic_locking,
	buffered_bulk // DEFAULT
};

template <unsigned int DIM, unsigned int WIDTH>
class InsertionThreadPool {
public:

	InsertionThreadPool(size_t furtherThreads,
			const std::vector<std::vector<unsigned long>>& values,
			const std::vector<int>& ids, PHTree<DIM, WIDTH>* tree);
	~InsertionThreadPool();
	void joinPool();

	const size_t fixRangeSize = 100;
	static InsertionOrder order_;
	static InsertionApproach approach_;

	static unsigned long nFlushPhases;

private:

	bool poolSyncRequired_;
	std::atomic<unsigned int> i_;
	size_t nThreads_;
	size_t currentBarrierSize;
	std::atomic<size_t> nRemainingThreads_;
	mutex createBarriersMutex_;
	boost::barrier* poolFlushBarrier_;
	std::vector<std::thread> threads_;
	const std::vector<std::vector<unsigned long>>& values_;
	const std::vector<int>& ids_;
	PHTree<DIM, WIDTH>* tree_;

	void processNext(size_t threadIndex);
	inline void insertBySelectedStrategy(size_t entryIndex, EntryBufferPool<DIM,WIDTH>* pool);

	inline void handlePoolFlushSync(EntryBufferPool<DIM,WIDTH>* pool, bool lastFlush);
	inline void handleDoneBySelectedStrategy(EntryBufferPool<DIM,WIDTH>* pool);

};

#include "Entry.h"
#include "util/DynamicNodeOperationsUtil.h"
#include "util/NodeTypeUtil.h"
#include "util/EntryBufferPool.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
InsertionOrder InsertionThreadPool<DIM, WIDTH>::order_ = range_per_thread;
template <unsigned int DIM, unsigned int WIDTH>
InsertionApproach InsertionThreadPool<DIM, WIDTH>::approach_ = buffered_bulk;
template <unsigned int DIM, unsigned int WIDTH>
unsigned long InsertionThreadPool<DIM, WIDTH>::nFlushPhases = 0;

template <unsigned int DIM, unsigned int WIDTH>
InsertionThreadPool<DIM, WIDTH>::InsertionThreadPool(size_t furtherThreads,
		const vector<vector<unsigned long>>& values,
		const vector<int>& ids, PHTree<DIM, WIDTH>* tree)
		: poolSyncRequired_(false), i_(0), nThreads_(furtherThreads + 1), createBarriersMutex_(),
		  poolFlushBarrier_(NULL), values_(values),
		  ids_(ids), tree_(tree) {
	assert (values.size() > 0);
	// create the biggest possible root node so there is no need to synchronize access on the root
	Node<DIM>* oldRoot = tree->root_;
	assert (oldRoot->getNumberOfContents() == 0);
	Node<DIM>* newRoot = NodeTypeUtil<DIM>::copyIntoLargerNode(1uL << DIM, oldRoot);
	tree->root_ = newRoot;
	delete oldRoot;

	poolFlushBarrier_ = new boost::barrier(nThreads_);
	currentBarrierSize = nThreads_;

	DynamicNodeOperationsUtil<DIM,WIDTH>::nThreads = nThreads_;
	nRemainingThreads_ = nThreads_;
	threads_.reserve(furtherThreads);
	for (unsigned tCount = 0; tCount < furtherThreads; ++tCount) {
		threads_.emplace_back(&InsertionThreadPool<DIM,WIDTH>::processNext, this, tCount);
	}
}

template <unsigned int DIM, unsigned int WIDTH>
InsertionThreadPool<DIM, WIDTH>::~InsertionThreadPool() {
	for (auto &t : threads_) {
		t.join();
	}

	assert (nRemainingThreads_ == 0);
	delete poolFlushBarrier_;

	// shrink the root node again
	size_t rootContents = tree_->root_->getNumberOfContents();
	assert (rootContents > 0);
	Node<DIM>* oldRoot = tree_->root_;
	Node<DIM>* newRoot = NodeTypeUtil<DIM>::copyIntoLargerNode(rootContents, oldRoot);
	tree_->root_ = newRoot;
	delete oldRoot;
}

template <unsigned int DIM, unsigned int WIDTH>
void InsertionThreadPool<DIM, WIDTH>::joinPool() {
	processNext(threads_.size());
}

template <unsigned int DIM, unsigned int WIDTH>
void InsertionThreadPool<DIM, WIDTH>::handlePoolFlushSync(EntryBufferPool<DIM,WIDTH>* pool, bool lastFlush) {
	// decide who creates the barriers
	createBarriersMutex_.lock(); // needs shared_lock?
	if (currentBarrierSize != nRemainingThreads_) {
		delete poolFlushBarrier_;
		poolFlushBarrier_ = new boost::barrier(nRemainingThreads_);
		const size_t nRemainingThreads = nRemainingThreads_;
		currentBarrierSize = nRemainingThreads;
	}
	createBarriersMutex_.unlock();

	const bool responsibleForState = poolFlushBarrier_->wait();
	pool->fullDeallocate();
	if (responsibleForState) {
		++nFlushPhases;
		poolSyncRequired_ = false;
	}

	if (lastFlush) { --nRemainingThreads_; }
	poolFlushBarrier_->wait();
}

template <unsigned int DIM, unsigned int WIDTH>
void InsertionThreadPool<DIM, WIDTH>::handleDoneBySelectedStrategy(EntryBufferPool<DIM,WIDTH>* pool) {
	switch (approach_) {
	case buffered_bulk:
		handlePoolFlushSync(pool, true);
		break;
	case optimistic_locking:
		--nRemainingThreads_;
		break;
	default:
		break;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void InsertionThreadPool<DIM, WIDTH>::insertBySelectedStrategy(size_t index, EntryBufferPool<DIM,WIDTH>* pool) {
	const Entry<DIM, WIDTH> entry(values_[index], ids_[index]);
	switch (approach_) {
	case optimistic_locking:
		DynamicNodeOperationsUtil<DIM, WIDTH>::parallelInsert(entry, *tree_);
		break;
	case buffered_bulk:
		bool success = false;
		while (!success) {
			if (poolSyncRequired_) { handlePoolFlushSync(pool, false); }
			success = DynamicNodeOperationsUtil<DIM, WIDTH>::parallelBulkInsert(entry, *tree_, *pool);
			if (!success) { poolSyncRequired_ = true; }
		}
		break;
	}
}

template <unsigned int DIM, unsigned int WIDTH>
void InsertionThreadPool<DIM, WIDTH>::processNext(size_t threadIndex) {
	const size_t size = values_.size();
	EntryBufferPool<DIM, WIDTH>* pool = NULL;
	if (approach_ == buffered_bulk) {
		pool = new EntryBufferPool<DIM, WIDTH>();
	}

	switch (order_) {
	case sequential_entries:
		{
			size_t i = 0;
			while (i < size) {
				i = i_++;
				if (i < size) {
					insertBySelectedStrategy(i, pool);
				}
			}
		}
		break;
	case range_per_thread:
		{
			const size_t start = size * threadIndex / nThreads_;
			const size_t end = min(size * (threadIndex + 1) / nThreads_, size);
			for (size_t i = start; i < end; ++i) {
				insertBySelectedStrategy(i, pool);
			}
		}
		break;
	case sequential_ranges:
		{
			size_t start, end;
			start = 0;
			while (start < size) {
				const size_t blockIndex = i_++;
				start = blockIndex * fixRangeSize;
				end = min(start + fixRangeSize, size);
				for (size_t i = start; i < end; ++i) {
					insertBySelectedStrategy(i, pool);
				}
			}
		}
		break;
	default: throw "unknown order";
	}

	handleDoneBySelectedStrategy(pool);
	delete pool;
#ifdef PRINT
		cout << "thread (ID: " << threadIndex << ") finished" << endl;
#endif
}


#endif /* SRC_UTIL_INSERTIONTHREADPOOL_H_ */
