/*
 * AtomicAtomicSuffixStorage.h
 *
 *  Created on: Aug 17, 2016
 *      Author: max
 */

#ifndef SRC_NODES_ATOMICSUFFIXSTORAGE_H_
#define SRC_NODES_ATOMICSUFFIXSTORAGE_H_

#include <atomic>

template <unsigned int DIM>
class SizeVisitor;

template <unsigned int SUFFIX_BLOCKS>
class AtomicSuffixStorage : public TSuffixStorage {
	template <unsigned int D>
	friend class SizeVisitor;
public:
	AtomicSuffixStorage();
	virtual ~AtomicSuffixStorage() {};
	bool canStoreBits(size_t nBitsToStore) const override;
	unsigned int getTotalBlocksToStoreAdditionalSuffix(size_t nSuffixBits) const override;
	std::pair<unsigned long*, unsigned int> reserveBits(size_t nBits) override;
	unsigned int overrideBlocksWithLast(size_t nBits, unsigned int overrideStartBlockIndex) override;
	void copyFrom(const TSuffixStorage& other) override;
	void clear() override;
	void clear(unsigned long* startBlock) override;
	void clearLast(size_t nBits) override;
	unsigned int getNMaxStorageBlocks() const override;
	unsigned int getNCurrentStorageBlocks() const override;
	unsigned long getBlock(unsigned int index) const override;
	bool empty() const override;
	unsigned long* getPointerFromIndex(unsigned int index) const override;
	unsigned int getIndexFromPointer(const unsigned long* pointer) const override;
	size_t getByteSize() const override;
	void setIndexUsed(size_t index) override;

private:

	typedef unsigned char byte;
	static const byte FULLY_OCCUPIED = ~0;
	static const size_t N_BITS_PER_OCCUPIED_BLOCK = 8 * sizeof (byte);
	static const size_t N_OCCUPIED_BLOCKS = 1 + (SUFFIX_BLOCKS - 1) / N_BITS_PER_OCCUPIED_BLOCK;

	unsigned int nBlocksPerSuffix_;
	std::atomic<byte> occupiedBlocks_[N_OCCUPIED_BLOCKS];
	unsigned long suffixBlocks_[SUFFIX_BLOCKS];

	size_t indexOfFreeBlock(byte occupiedBlock) const;
	inline size_t popcount(byte b) const;
};

using namespace std;
#include <assert.h>

template <unsigned int SUFFIX_BLOCKS>
AtomicSuffixStorage<SUFFIX_BLOCKS>::AtomicSuffixStorage() : nBlocksPerSuffix_(0), occupiedBlocks_(), suffixBlocks_() { }

template <unsigned int SUFFIX_BLOCKS>
unsigned int  AtomicSuffixStorage<SUFFIX_BLOCKS>::getNMaxStorageBlocks() const {
	return SUFFIX_BLOCKS;
}

template <unsigned int SUFFIX_BLOCKS>
unsigned int AtomicSuffixStorage<SUFFIX_BLOCKS>::getNCurrentStorageBlocks() const {
	size_t nOccupiedBlocks = 0;
	for (unsigned i = 0; i < N_OCCUPIED_BLOCKS; ++i) {
		const byte block = occupiedBlocks_[i];
		nOccupiedBlocks += popcount(block);
	}

	return nOccupiedBlocks * nBlocksPerSuffix_;
}

template <unsigned int SUFFIX_BLOCKS>
inline size_t AtomicSuffixStorage<SUFFIX_BLOCKS>::popcount(byte b) const {
	size_t count = 0;
	for (unsigned i = 0; i < N_BITS_PER_OCCUPIED_BLOCK; ++i) {
		count += (b >> i) & 1;
	}

	return count;
}

template <unsigned int SUFFIX_BLOCKS>
bool AtomicSuffixStorage<SUFFIX_BLOCKS>::empty() const {
	for (unsigned i = 0; i < N_OCCUPIED_BLOCKS; ++i) {
		if (occupiedBlocks_[i] != 0) {
			return false;
		}
	}

	return true;
}

template <unsigned int SUFFIX_BLOCKS>
void AtomicSuffixStorage<SUFFIX_BLOCKS>::setIndexUsed(size_t index) {
	const size_t blockIndex = index / N_BITS_PER_OCCUPIED_BLOCK;
	const byte occupiedBlock = occupiedBlocks_[blockIndex];
	const size_t occupiedBit = index % N_BITS_PER_OCCUPIED_BLOCK;
	occupiedBlocks_[blockIndex] = occupiedBlock | (1u << occupiedBit);
}

template <unsigned int SUFFIX_BLOCKS>
unsigned long AtomicSuffixStorage<SUFFIX_BLOCKS>::getBlock(unsigned int index) const {
	return suffixBlocks_[index];
}

template <unsigned int SUFFIX_BLOCKS>
unsigned long* AtomicSuffixStorage<SUFFIX_BLOCKS>::getPointerFromIndex(unsigned int index) const {
	return const_cast<unsigned long*>(suffixBlocks_ + index);
}

template <unsigned int SUFFIX_BLOCKS>
unsigned int AtomicSuffixStorage<SUFFIX_BLOCKS>::getIndexFromPointer(const unsigned long* pointer) const {
	const size_t pointerRepr = (size_t)pointer;
	const size_t startRepr = (size_t)suffixBlocks_;
	const size_t diff = pointerRepr - startRepr;
	assert (diff < (1uL << (8 * sizeof (unsigned int))));
	const unsigned int index = diff / (sizeof (unsigned long));
	assert ((*pointer) == suffixBlocks_[index]);
	return index;
}

template <unsigned int SUFFIX_BLOCKS>
void AtomicSuffixStorage<SUFFIX_BLOCKS>::copyFrom(const TSuffixStorage& other) {
	assert (SUFFIX_BLOCKS >= other.getNCurrentStorageBlocks());
	for (unsigned int block = 0; block < SUFFIX_BLOCKS; ++block) {
		suffixBlocks_[block] = other.getBlock(block);
	}
}

template <unsigned int SUFFIX_BLOCKS>
void AtomicSuffixStorage<SUFFIX_BLOCKS>::clear() {
	// TODO can be shortened by looking at occupied blocks first and deleting conditionally
	nBlocksPerSuffix_ = 0;
	for (unsigned i = 0; i < N_OCCUPIED_BLOCKS; ++i) {
		occupiedBlocks_[i] = 0;
	}

	for (unsigned i = 0; i < SUFFIX_BLOCKS; ++i) {
		suffixBlocks_[i] = 0;
	}
}

template <unsigned int SUFFIX_BLOCKS>
void AtomicSuffixStorage<SUFFIX_BLOCKS>::clear(unsigned long* startBlock) {

	assert (nBlocksPerSuffix_ != 0);
	size_t index = getIndexFromPointer(startBlock) / nBlocksPerSuffix_;
	const size_t occupiedBlockIndex = index / N_BITS_PER_OCCUPIED_BLOCK;
	const size_t occupiedBlockBitIndex = index % N_BITS_PER_OCCUPIED_BLOCK;

	byte occupiedBlock = occupiedBlocks_[occupiedBlockIndex];
	byte occupiedBlockShould;
	do {
		occupiedBlockShould = occupiedBlock & (~(1 << occupiedBlockBitIndex));
		assert  (occupiedBlockShould != occupiedBlock);
	} while (!atomic_compare_exchange_strong(&occupiedBlocks_[occupiedBlockIndex], &occupiedBlock, occupiedBlockShould));
}

template <unsigned int SUFFIX_BLOCKS>
void AtomicSuffixStorage<SUFFIX_BLOCKS>::clearLast(size_t nBits) {
	throw "unsupported";
}

template <unsigned int SUFFIX_BLOCKS>
unsigned int AtomicSuffixStorage<SUFFIX_BLOCKS>::overrideBlocksWithLast(size_t nBits, unsigned int overrideStartBlockIndex) {
	throw "unsupported";
}

template <unsigned int SUFFIX_BLOCKS>
unsigned int AtomicSuffixStorage<SUFFIX_BLOCKS>::getTotalBlocksToStoreAdditionalSuffix(size_t nSuffixBits) const {
	// never needs to enlarge the storage as it is required to have the maximum possible size
	return 0;
}

template <unsigned int SUFFIX_BLOCKS>
bool AtomicSuffixStorage<SUFFIX_BLOCKS>::canStoreBits(size_t nBitsToStore) const {
	return true;
}

template <unsigned int SUFFIX_BLOCKS>
pair<unsigned long*, unsigned int> AtomicSuffixStorage<SUFFIX_BLOCKS>::reserveBits(size_t nBits) {
	if (nBlocksPerSuffix_ == 0) {
		nBlocksPerSuffix_ = 1 + (nBits - 1) / (8 * sizeof (unsigned long));
	}

	size_t occupiedBlockIndex = 0;
	byte currentOccupiedBlock = occupiedBlocks_[0];
	byte newOccupiedBlock;
	unsigned int startBlockIndex;
	do {
		while (currentOccupiedBlock == FULLY_OCCUPIED) {
			// go to next block that indicates a free block remaining
			occupiedBlockIndex = (occupiedBlockIndex + 1) % N_OCCUPIED_BLOCKS;
			currentOccupiedBlock = occupiedBlocks_[occupiedBlockIndex];
		}

		// mark any free block as used
		startBlockIndex = indexOfFreeBlock(currentOccupiedBlock);
		newOccupiedBlock = currentOccupiedBlock | (1 << startBlockIndex);
		assert (newOccupiedBlock != currentOccupiedBlock);
	} while (!atomic_compare_exchange_strong(&occupiedBlocks_[occupiedBlockIndex], &currentOccupiedBlock, newOccupiedBlock));

	startBlockIndex += N_BITS_PER_OCCUPIED_BLOCK * occupiedBlockIndex;
	startBlockIndex = startBlockIndex * nBlocksPerSuffix_;
	unsigned long* reservedStartBlock = suffixBlocks_ + startBlockIndex;
	return pair<unsigned long*, unsigned int>(reservedStartBlock, startBlockIndex);
}

template <unsigned int SUFFIX_BLOCKS>
size_t AtomicSuffixStorage<SUFFIX_BLOCKS>::indexOfFreeBlock(byte occupiedBlock) const {
	size_t index = 0;
	while (occupiedBlock & 1) {
		occupiedBlock >>= 1;
		index += 1;
	}

	return index;
}

template <unsigned int SUFFIX_BLOCKS>
size_t AtomicSuffixStorage<SUFFIX_BLOCKS>::getByteSize() const {
	size_t byteSize = sizeof (nBlocksPerSuffix_);
	byteSize += sizeof (suffixBlocks_);
	byteSize += sizeof (occupiedBlocks_);
	return byteSize;
}

#endif /* SRC_NODES_ATOMICSUFFIXSTORAGE_H_ */
