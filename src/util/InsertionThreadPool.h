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

template <unsigned int DIM, unsigned int WIDTH>
class PHTree;

enum InsertionOrder {
		sequential_entries,
		range_per_thread,
		sequential_ranges
};

template <unsigned int DIM, unsigned int WIDTH>
class InsertionThreadPool {
public:

	InsertionThreadPool(size_t nThreads,
			const std::vector<std::vector<unsigned long>>& values,
			const std::vector<int>* ids, PHTree<DIM, WIDTH>* tree);
	~InsertionThreadPool();
	void joinPool();

	const size_t fixRangeSize = 100;
	static InsertionOrder order_;
private:

	std::atomic<unsigned int> i_;
	mutex m;
	size_t nThreads_;
	std::vector<std::thread> threads_;
	const std::vector<std::vector<unsigned long>>& values_;
	const std::vector<int>* ids_;
	PHTree<DIM, WIDTH>* tree_;

	void processNext(size_t threadIndex);

};

#include "Entry.h"
#include "util/DynamicNodeOperationsUtil.h"
#include "util/NodeTypeUtil.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
InsertionOrder InsertionThreadPool<DIM, WIDTH>::order_ = range_per_thread;

template <unsigned int DIM, unsigned int WIDTH>
InsertionThreadPool<DIM, WIDTH>::InsertionThreadPool(size_t nThreads,
		const vector<vector<unsigned long>>& values,
		const vector<int>* ids, PHTree<DIM, WIDTH>* tree)
		: i_(0), values_(values), ids_(ids), tree_(tree) {
	assert (values.size() > 0);
	// create the biggest possible root node so there is no need to synchronize access on the root
	Node<DIM>* oldRoot = tree->root_;
	assert (oldRoot->getNumberOfContents() == 0);
	Node<DIM>* newRoot = NodeTypeUtil<DIM>::copyIntoLargerNode(1uL << DIM, oldRoot);
	tree->root_ = newRoot;
	delete oldRoot;

	nThreads_ = nThreads + 1;
	DynamicNodeOperationsUtil<DIM,WIDTH>::nThreads = nThreads_;
	threads_.reserve(nThreads);
	for (unsigned tCount = 0; tCount < nThreads; ++tCount) {
		threads_.emplace_back(&InsertionThreadPool<DIM,WIDTH>::processNext, this, tCount);
	}
}

template <unsigned int DIM, unsigned int WIDTH>
InsertionThreadPool<DIM, WIDTH>::~InsertionThreadPool() {
	for (auto &t : threads_) {
		t.join();
	}

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
void InsertionThreadPool<DIM, WIDTH>::processNext(size_t threadIndex) {
	const size_t size = values_.size();

	switch (order_) {
	case sequential_entries: {
		size_t i = 0;
		while (i < size) {
			i = i_++;
			if (i < size) {
				const int id = (ids_)? (*ids_)[i] : int(i);
				const Entry<DIM, WIDTH> entry(values_[i], id);
				DynamicNodeOperationsUtil<DIM, WIDTH>::parallelInsert(entry, *tree_);
			}
		}
	}

		break;
	case range_per_thread: {
		const size_t start = size * threadIndex / nThreads_;
		const size_t end = min(size * (threadIndex + 1) / nThreads_, size);
		for (size_t i = start; i < end; ++i) {
			// get the next valid work item
			const int id = (ids_)? (*ids_)[i] : int(i);
			const Entry<DIM, WIDTH> entry(values_[i], id);
			DynamicNodeOperationsUtil<DIM, WIDTH>::parallelInsert(entry, *tree_);
		}
	}

		break;
	case sequential_ranges: {
		size_t start, end;
		start = 0;
		while (start < size) {
			const size_t blockIndex = i_++;
			start = blockIndex * fixRangeSize;
			end = min(start + fixRangeSize, size);
			for (size_t i = start; i < end; ++i) {
				// get the next valid work item
				const int id = (ids_)? (*ids_)[i] : int(i);
				const Entry<DIM, WIDTH> entry(values_[i], id);
				DynamicNodeOperationsUtil<DIM, WIDTH>::parallelInsert(entry, *tree_);
			}
		}
	}
		break;
	default: throw "unknown order";
	}

#ifdef PRINT
		cout << "thread (ID: " << threadIndex << ") finished" << endl;
#endif
}


#endif /* SRC_UTIL_INSERTIONTHREADPOOL_H_ */
