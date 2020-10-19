// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <assert.h>
#include <algorithm>
#include <vector>

#define SUPPORTS_ALIGNMENT
#define SUPPORTS_SHOWFREEBLOCKS
#define SUPPORTS_SHOWOUTSTANDINGALLOCATIONS

//extern bool HeapManager_UnitTest();

//bool HeapManager_UnitTest();

struct BlockDescriptor {
	uint8_t* startMemBlockAddr;
	size_t sizeOfBlock;
	BlockDescriptor* nextBlock;
	BlockDescriptor* prevBlock;
};

class HeapManager {

	public:

		static HeapManager* create(void* heapMemory, size_t heapMemorySize) {

			HeapManager* heapManager = (HeapManager*)heapMemory;
			heapManager->startOfHeap = reinterpret_cast<uint8_t*>(heapMemory) + sizeof(HeapManager);
			
			BlockDescriptor* temp = reinterpret_cast<BlockDescriptor*>(heapManager->startOfHeap);
			temp->sizeOfBlock = heapMemorySize - sizeof(HeapManager) - sizeof(BlockDescriptor);
			temp->startMemBlockAddr = reinterpret_cast<uint8_t*>(heapManager->startOfHeap) + sizeof(BlockDescriptor);
			temp->prevBlock = nullptr;
			temp->nextBlock = nullptr;
			heapManager->FreeBlocks = temp;

			heapManager->sizeOfHeap = heapMemorySize - sizeof(HeapManager);

			return heapManager;

		};

		void* _alloc(size_t i_bytes, unsigned int i_alignment) {
			
			size_t guardbanding = 4;

			size_t allignedAllocRequest;

			//fixes alignment of alloc request
			if (i_bytes % i_alignment == 0) {
				allignedAllocRequest = i_bytes;
			}
			else {
				allignedAllocRequest = i_bytes % i_alignment + i_bytes;
			}

			BlockDescriptor* temp = this->FreeBlocks;
			//makes sure there is a free block big eniugh to satisfy alloc request
			while (temp != nullptr) {
				if (temp->sizeOfBlock >= i_bytes)
					break;
				temp = temp->nextBlock;
			}
			//a big enough free block was found that was big enough to satisfy alloc request
			if (temp)
			{
				size_t prevBlockSize;
				prevBlockSize = temp->sizeOfBlock;
				

				if (temp->prevBlock != nullptr && temp->nextBlock!=nullptr)
				{
					temp->prevBlock->nextBlock = temp->nextBlock;
					temp->nextBlock->prevBlock = temp->prevBlock;
				}
				else if(temp->prevBlock != nullptr)
				{
					temp->prevBlock->nextBlock = temp->nextBlock;
				}
				else
				{
					this->FreeBlocks = this->FreeBlocks->nextBlock;
				}

				if (this->AllocatedBlocks != nullptr)
				{
					this->AllocatedBlocks->prevBlock = temp;
				}
				temp->prevBlock = nullptr;
				temp->nextBlock = this->AllocatedBlocks;
				this->AllocatedBlocks = temp;
				
				if ((prevBlockSize - allignedAllocRequest - sizeof(BlockDescriptor) > sizeof(BlockDescriptor)))
				{
					temp->sizeOfBlock = allignedAllocRequest;
					BlockDescriptor* newFreeBlock = (BlockDescriptor*)temp->startMemBlockAddr + i_bytes + guardbanding;
					newFreeBlock->sizeOfBlock = prevBlockSize - i_bytes - sizeof(BlockDescriptor);
					newFreeBlock->startMemBlockAddr = reinterpret_cast<uint8_t*>(newFreeBlock) + sizeof(BlockDescriptor);

					if (this->FreeBlocks != nullptr)
					{
						newFreeBlock->nextBlock = this->FreeBlocks;
						newFreeBlock->prevBlock = this->FreeBlocks->prevBlock;
						this->FreeBlocks->prevBlock = newFreeBlock;

						this->FreeBlocks = newFreeBlock;

					}
					else
					{
						this->FreeBlocks = newFreeBlock;
						this->FreeBlocks->nextBlock = nullptr;
						this->FreeBlocks->prevBlock = nullptr;
					}
				}

				return this->AllocatedBlocks->startMemBlockAddr;
			}
			//no big enough free block was found to satisfy alloc request
			else {
				printf("Allocation request cannot be satisfied");
				return NULL;
			}
			
		};

		void _free(void* i_ptr) {

			BlockDescriptor* tempAlloc = this->AllocatedBlocks;
			BlockDescriptor* tempFree = this->FreeBlocks;
			//finding a block in the allocated block list with the same startMemBlockAddr as i_ptr
			while (tempAlloc != nullptr) {
				if (tempAlloc->startMemBlockAddr == i_ptr)
					break;
				tempAlloc = tempAlloc->nextBlock;
			}
			//the previous while loop found the correct tempAlloc block that should be freed
			if (tempAlloc) {
				//if tempAlloc is the head of the alloc list
				if (tempAlloc == this->AllocatedBlocks)
				{
					//tempAlloc is the head and there is more than one block in the alloc list
					if (this->AllocatedBlocks->nextBlock != nullptr) {
						this->AllocatedBlocks = this->AllocatedBlocks->nextBlock;
						this->AllocatedBlocks->prevBlock = nullptr;
						tempAlloc->prevBlock = nullptr;
						tempAlloc->nextBlock = nullptr;
					}
					//tempAlloc is the head and there is only one block in the alloc list
					else {
						this->AllocatedBlocks = this->AllocatedBlocks->nextBlock;
						tempAlloc->prevBlock = nullptr;
						tempAlloc->nextBlock = nullptr;
					}
					
					
				}
				//if tempAlloc is not the head of the alloc list
				else {
					//if there exists a block before tempAlloc
					if (tempAlloc->prevBlock != nullptr)
					{
						//if tempAlloc has a next block
						if (tempAlloc->nextBlock != nullptr)
						{
							//set tempAlloc's prev block's next block in allocated list to temp's next block
							tempAlloc->prevBlock->nextBlock = tempAlloc->nextBlock;
						}
						//if tempAlloc does not have a next block
						else 
						{
							//set temp's prev block's next block to null since tempAlloc has no next block in the allocated list
							tempAlloc->prevBlock->nextBlock = nullptr;
						}
					}
					//if there exists a block after tempAlloc
					if (tempAlloc->nextBlock != nullptr)
					{
						//set temp's next block in allocated list to temp's prev block
						tempAlloc->nextBlock->prevBlock = tempAlloc->prevBlock;
					}
				}
				
				

				while (tempFree != nullptr) {
					//find the right place to put this newly freed tempAlloc block in the free list (insertion sort)
					if (tempAlloc->startMemBlockAddr + tempAlloc->sizeOfBlock < reinterpret_cast<uint8_t*>(tempFree)) {
						//if tempFree is not the head of the free list
						if (tempFree->prevBlock != nullptr)
						{
							BlockDescriptor* oldTempFreePrevBlock = tempFree->prevBlock;
							tempFree->prevBlock = tempAlloc;
							oldTempFreePrevBlock->nextBlock = tempAlloc;
							tempAlloc->nextBlock = tempFree;
							tempAlloc->prevBlock = oldTempFreePrevBlock;
							break;
						}
						else {
							tempFree->prevBlock = tempAlloc;
							tempAlloc->nextBlock = tempFree;
							tempAlloc->prevBlock = nullptr;
							this->FreeBlocks = tempAlloc;
							break;
						}
						
					}
					tempFree = tempFree->nextBlock;
				}
			}
			else {
				printf("Invalid free request\n");
			}
		};

		// attempt to merge abutting blocks.
		void Collect() {

			BlockDescriptor* tempFree = this->FreeBlocks;

			while (tempFree->nextBlock != nullptr)
			{
				if ((tempFree->startMemBlockAddr + tempFree->sizeOfBlock) < reinterpret_cast<uint8_t*>(tempFree->nextBlock)) {
					if (tempFree->nextBlock->nextBlock != nullptr)
					{
						tempFree->sizeOfBlock += tempFree->nextBlock->sizeOfBlock;
						tempFree->nextBlock->nextBlock->prevBlock = tempFree;
						tempFree->nextBlock = tempFree->nextBlock->nextBlock;
					}
					else {
						tempFree->sizeOfBlock += tempFree->nextBlock->sizeOfBlock;
						tempFree->nextBlock = nullptr;
					}
				}
			}
		};

		bool Contains(void * i_ptr) {

			/*BlockDescriptor* tempAlloc = this->AllocatedBlocks;
			BlockDescriptor* tempFree = this->FreeBlocks;
			while (tempAlloc != nullptr) {
				if (tempAlloc->startMemBlockAddr == reinterpret_cast<uint8_t*>(i_ptr)) {
					return 1;
				}
				tempAlloc = tempAlloc->nextBlock;
			}
			while (tempFree != nullptr) {
				if (tempFree->startMemBlockAddr == reinterpret_cast<uint8_t*>(i_ptr)) {
					return 1;
				}
				tempFree = tempFree->nextBlock;
			}*/

			return ((i_ptr >= this->startOfHeap) && (i_ptr <= (this->startOfHeap + this->sizeOfHeap)));
		};

		bool IsAllocated(void * i_ptr) {

			BlockDescriptor* temp = this->AllocatedBlocks;

			while (temp != nullptr) {
				if (temp->startMemBlockAddr == reinterpret_cast<uint8_t*>(i_ptr)) {
					return 1;
				}
				temp = temp->nextBlock;
			}

			return 0;
		};

		size_t GetLargestFreeBlock() {
			
			size_t largestFreeBlock = 0;

			if (this->FreeBlocks != nullptr)
			{
				BlockDescriptor* temp = this->FreeBlocks;

				while (temp != nullptr) {
					if (temp->sizeOfBlock > largestFreeBlock) {
						largestFreeBlock = temp->sizeOfBlock;
					}
					temp = temp->nextBlock;
				}
				return largestFreeBlock;
			}
			else 
			{
				printf("No Free Blocks!\n");
				return NULL;
			}

		};

		size_t GetTotalFreeMemory() {

			if (this->FreeBlocks != nullptr)
			{
				size_t totalFreeMemory = 0;
				BlockDescriptor* temp = this->FreeBlocks;

				while (temp != nullptr) {
					totalFreeMemory += temp->sizeOfBlock;
					temp = temp->nextBlock;
				}
				return totalFreeMemory;
			}
			else
			{
				return 0;
			}
		}

		void ShowFreeBlocks() {
			BlockDescriptor* temp = this->FreeBlocks;

			while (temp != nullptr) {
				printf("FreeBlock Size: %d ", temp->sizeOfBlock);
				printf("FreeBlock Address: %d\n", temp->startMemBlockAddr);
				temp = temp->nextBlock;
			}
		};

		void ShowOutstandingAllocations() {
			BlockDescriptor* temp = this->AllocatedBlocks;

			while (temp != nullptr) {
				printf("AllocatedBlock Size: %d ", temp->sizeOfBlock);
				printf("AllocatedBlock Address: %d\n", temp->startMemBlockAddr);
				temp = temp->nextBlock;
			}
		};


	private:
		//a head for free blocks linked list
		BlockDescriptor* FreeBlocks;
		//a head for allocated blocks linked list
		BlockDescriptor* AllocatedBlocks;
		//starting address of heap
		uint8_t* startOfHeap;
		//size of heap
		size_t sizeOfHeap;

};


int main()
{
	const size_t 		sizeHeap = 1024 * 1024;
	const unsigned int 	numDescriptors = 2048;

	void* pHeapMemory = HeapAlloc(GetProcessHeap(), 0, sizeHeap);

	HeapManager* i_ptr = HeapManager::create(pHeapMemory, sizeHeap);

	int* p= (int*)i_ptr->_alloc(13, 16);

	int* q = (int*)i_ptr->_alloc(33, 32);

	int* r = (int*)i_ptr->_alloc(1, 4);

	for (int i = 0; i < 10; i++)
	{
		p[i] = i;
	}

	printf("\n");

	for (int i = 0; i < 10; i++) {
		printf("%d ", i);
	}

	printf("\n\n");

	i_ptr->_free(r);


	i_ptr->_free(p);


	i_ptr->_free(q);

	i_ptr->ShowFreeBlocks();

	printf("\n");

	printf("Total Free Memory: %d\n", static_cast<int>(i_ptr->GetTotalFreeMemory()));

	printf("\n");

	printf("Largest Free Block: %d\n", static_cast<int>(i_ptr->GetLargestFreeBlock()));

	i_ptr->Collect();

	printf("\n");

	i_ptr->ShowFreeBlocks();

	printf("\n");

	i_ptr->ShowOutstandingAllocations();
}
/*
bool HeapManager_UnitTest()
{
	using namespace HeapManagerHandler;
	const size_t 		sizeHeap = 1024 * 1024;
	const unsigned int 	numDescriptors = 2048;
#ifdef USE_HEAP_ALLOC
	void* pHeapMemory = HeapAlloc(GetProcessHeap(), 0, sizeHeap);
#else
	// Get SYSTEM_INFO, which includes the memory page size
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	// round our size to a multiple of memory page size
	assert(SysInfo.dwPageSize > 0);
	size_t sizeHeapInPageMultiples = SysInfo.dwPageSize * ((sizeHeap + SysInfo.dwPageSize) / SysInfo.dwPageSize);
	void* pHeapMemory = VirtualAlloc(NULL, sizeHeapInPageMultiples, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
	assert(pHeapMemory);
	// Create a heap manager for my test heap.
	HeapManager * pHeapManager = CreateHeapManager(pHeapMemory, sizeHeap, numDescriptors);
	assert(pHeapManager);
	if (pHeapManager == nullptr)
		return false;
#ifdef TEST_SINGLE_LARGE_ALLOCATION
	// This is a test I wrote to check to see if using the whole block if it was almost consumed by 
	// an allocation worked. Also helped test my ShowFreeBlocks() and ShowOutstandingAllocations().
	{
#ifdef SUPPORTS_SHOWFREEBLOCKS
		ShowFreeBlocks(pHeapManager);
#endif // SUPPORTS_SHOWFREEBLOCKS
		size_t largestBeforeAlloc = GetLargestFreeBlock(pHeapManager);
		void * pPtr = alloc(pHeapManager, largestBeforeAlloc - HeapManager::s_MinumumToLeave);
		if (pPtr)
		{
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After large allocation:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			ShowFreeBlocks(pHeapManager);
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif
			size_t largestAfterAlloc = GetLargestFreeBlock(pHeapManager);
			bool success = Contains(pHeapManager, pPtr) && IsAllocated(pHeapManager, pPtr);
			assert(success);
			success = free(pHeapManager, pPtr);
			assert(success);
			Collect(pHeapManager);
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After freeing allocation and garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			ShowFreeBlocks(pHeapManager);
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif
			size_t largestAfterCollect = GetLargestFreeBlock(pHeapManager);
		}
	}
#endif
	std::vector<void *> AllocatedAddresses;
	long	numAllocs = 0;
	long	numFrees = 0;
	long	numCollects = 0;
	// allocate memory of random sizes up to 1024 bytes from the heap manager
	// until it runs out of memory
	do
	{
		const size_t		maxTestAllocationSize = 1024;
		size_t			sizeAlloc = 1 + (rand() & (maxTestAllocationSize - 1));
#ifdef SUPPORTS_ALIGNMENT
		// pick an alignment
		const unsigned int	alignments[] = { 4, 8, 16, 32, 64 };
		const unsigned int	index = rand() % (sizeof(alignments) / sizeof(alignments[0]));
		const unsigned int	alignment = alignments[index];
		void * pPtr = alloc(pHeapManager, sizeAlloc, alignment);
		// check that the returned address has the requested alignment
		assert((reinterpret_cast<uintptr_t>(pPtr) & (alignment - 1)) == 0);
#else
		void * pPtr = alloc(pHeapManager, sizeAlloc);
#endif // SUPPORT_ALIGNMENT
		// if allocation failed see if garbage collecting will create a large enough block
		if (pPtr == nullptr)
		{
			Collect(pHeapManager);
#ifdef SUPPORTS_ALIGNMENT
			pPtr = alloc(pHeapManager, sizeAlloc, alignment);
#else
			pPtr = alloc(pHeapManager, sizeAlloc);
#endif // SUPPORT_ALIGNMENT
			// if not we're done. go on to cleanup phase of test
			if (pPtr == nullptr)
				break;
		}
		AllocatedAddresses.push_back(pPtr);
		numAllocs++;
		// randomly free and/or garbage collect during allocation phase
		const unsigned int freeAboutEvery = 10;
		const unsigned int garbageCollectAboutEvery = 40;
		if (!AllocatedAddresses.empty() && ((rand() % freeAboutEvery) == 0))
		{
			void * pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();
			bool success = Contains(pHeapManager, pPtr) && IsAllocated(pHeapManager, pPtr);
			assert(success);
			success = free(pHeapManager, pPtr);
			assert(success);
			numFrees++;
		}
		if ((rand() % garbageCollectAboutEvery) == 0)
		{
			Collect(pHeapManager);
			numCollects++;
		}
	} while (1);
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
	printf("After exhausting allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
	ShowFreeBlocks(pHeapManager);
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	printf("\n");
#endif
	// now free those blocks in a random order
	if (!AllocatedAddresses.empty())
	{
		// randomize the addresses
		std::random_shuffle(AllocatedAddresses.begin(), AllocatedAddresses.end());
		// return them back to the heap manager
		while (!AllocatedAddresses.empty())
		{
			void * pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();
			bool success = Contains(pHeapManager, pPtr) && IsAllocated(pHeapManager, pPtr);
			assert(success);
			success = free(pHeapManager, pPtr);
			assert(success);
		}
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After freeing allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		ShowFreeBlocks(pHeapManager);
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif
		// do garbage collection
		Collect(pHeapManager);
		// our heap should be one single block, all the memory it started with
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		ShowFreeBlocks(pHeapManager);
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif
		// do a large test allocation to see if garbage collection worked
		void * pPtr = alloc(pHeapManager, sizeHeap / 2);
		assert(pPtr);
		if (pPtr)
		{
			bool success = Contains(pHeapManager, pPtr) && IsAllocated(pHeapManager, pPtr);
			assert(success);
			success = free(pHeapManager, pPtr);
			assert(success);
		}
	}
	Destroy(pHeapManager);
	pHeapManager = nullptr;
	if (pHeapMemory)
	{
#ifdef USE_HEAP_ALLOC
		HeapFree(GetProcessHeap(), 0, pHeapMemory);
#else
		VirtualFree(pHeapMemory, 0, MEM_RELEASE);
#endif
	}
	// we succeeded
	return true;
}
*/
