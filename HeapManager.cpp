// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <algorithm>
#include <vector>

#define SUPPORTS_ALIGNMENT
#define SUPPORTS_SHOWFREEBLOCKS
#define SUPPORTS_SHOWOUTSTANDINGALLOCATIONS

/**
* Used to store key information about each free or allocated memory block
*/
struct BlockDescriptor {
	uint8_t* startMemBlockAddr;     // reference to memory address of the start of actual memory block (not the block descriptor starting address)
	size_t sizeOfBlock;             // size of memory block (excluding block descriptor size and guardbanding)
	BlockDescriptor* nextBlock;     // reference to the next memory block
	BlockDescriptor* prevBlock;     // reference to the previous memory block
};

/**
* Creates the heap manager and handles all operations of allocating and freeing memory blocks 
* inside the heap manager
*/
class HeapManager {

	public:

        /**
        * Initializes the heap memory manager with one large heap memory block 
        *
        * @param heapMemory void pointer to hold address of heap manager.
        * @param heapMemorySize Total avaialble memory inside the heap manager.
        * @return A pointer to the heap manager that was initialised.
        */
		static HeapManager* HeapManager::create(void* heapMemory, size_t heapMemorySize) {

			HeapManager* heapManager = (HeapManager*)heapMemory;
			heapManager->startOfHeap = reinterpret_cast<uint8_t*>(heapMemory) + sizeof(HeapManager);
			
            // Initialising one large heap memory block to hold entire heap memory (excluding space set aside for BlockDescriptor struct attached to this block)
			BlockDescriptor* temp = reinterpret_cast<BlockDescriptor*>(heapManager->startOfHeap);
			temp->sizeOfBlock = heapMemorySize - sizeof(HeapManager) - sizeof(BlockDescriptor);
			temp->startMemBlockAddr = reinterpret_cast<uint8_t*>(heapManager->startOfHeap) + sizeof(BlockDescriptor);
			temp->prevBlock = nullptr;
			temp->nextBlock = nullptr;
			heapManager->FreeBlocks = temp;

			heapManager->sizeOfHeap = heapMemorySize - sizeof(HeapManager);

			return heapManager;

		};

        /**
        * Allocates a memory block of a given size if a free block of the given size or more exists in the heap manager 
        *
        * @param i_bytes void pointer to hold address of heap manager.
        * @param i_alignment Padding to ensure the returning allocated block's memory address is aligned with other already allocated blocks' memory address
        * @return A void pointer pointing to the starting memory address of the newly allocated memory block. NULL if a new block could not be allocated
        */
		void* HeapManager::_alloc(size_t i_bytes, unsigned int i_alignment) {
			
            //guardbanding to prevent memory overwrite at the start and end of each heap memory block
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
			//a big enough free block was found that was enough to satisfy alloc request
			if (temp)
			{
				size_t prevBlockSize;
				prevBlockSize = temp->sizeOfBlock;

                // if chosen free block is in the middle of the free block list
				if (temp->prevBlock != nullptr && temp->nextBlock != nullptr)
				{
                     //break all the links for this chosen free block from the free blocks list and update the links in the free blocks list accordingly
					temp->prevBlock->nextBlock = temp->nextBlock;
					temp->nextBlock->prevBlock = temp->prevBlock;
				}
                // if chosen free block is at the end of the free block list
				else if(temp->prevBlock != nullptr)
				{
                     //break all the links for this chosen free block from the free blocks list and update the links in the free blocks list accordingly
					temp->prevBlock->nextBlock = temp->nextBlock;
				}
                // if chosen free block is at the start of the free block list
				else
				{
                     //break all the links for this chosen free block from the free blocks list and update the links in the free blocks list accordingly
					this->FreeBlocks = this->FreeBlocks->nextBlock;
				}

                //add the chosen free block to the start of the allocated blocks list and setup the links accordingly 
				if (this->AllocatedBlocks != nullptr)
				{
					this->AllocatedBlocks->prevBlock = temp;
				}
				temp->prevBlock = nullptr;
				temp->nextBlock = this->AllocatedBlocks;
				this->AllocatedBlocks = temp;
				
                //Ensure that the chosen free block still satisfies the alloc request after alloc size request is adjusted for alignment
				if ((prevBlockSize - allignedAllocRequest - sizeof(BlockDescriptor) > sizeof(BlockDescriptor)))
				{
					temp->sizeOfBlock = allignedAllocRequest;
                    //since the chosen free block is big enough to satisfy alloc request even after alignment, add a new free block to the free list 
                    //accounting for the depleted free memory available after alloc request is successful 
					BlockDescriptor* newFreeBlock = (BlockDescriptor*)temp->startMemBlockAddr + i_bytes + guardbanding;
					newFreeBlock->sizeOfBlock = prevBlockSize - i_bytes - sizeof(BlockDescriptor);
					newFreeBlock->startMemBlockAddr = reinterpret_cast<uint8_t*>(newFreeBlock) + sizeof(BlockDescriptor);

                    // if the free block list is not empty
					if (this->FreeBlocks != nullptr)
					{
                        //adds this new free block at the head of the free block list and update links
						newFreeBlock->nextBlock = this->FreeBlocks;
						newFreeBlock->prevBlock = this->FreeBlocks->prevBlock;
						this->FreeBlocks->prevBlock = newFreeBlock;

						this->FreeBlocks = newFreeBlock;

					}
                    // if the free block list is empty
					else
					{
                        //adds this new free block at the head of the free block list and update links
						this->FreeBlocks = newFreeBlock;
						this->FreeBlocks->nextBlock = nullptr;
						this->FreeBlocks->prevBlock = nullptr;
					}
				}
                // returning memory address of the newly allocated block sitting at the head of the allocated block list
				return this->AllocatedBlocks->startMemBlockAddr;
			}
			//no big enough free block was found to satisfy alloc request
			else {
				printf("Allocation request cannot be satisfied");
				return NULL;
			}
			
		};

        /**
        * Frees a memory block at the given memory address 
        *
        * @param i_ptr void pointer to the address in heap memory where a block needs to be freed.
        */
		void HeapManager::_free(void* i_ptr) {

			BlockDescriptor* tempAlloc = this->AllocatedBlocks;
			BlockDescriptor* tempFree = this->FreeBlocks;
			//finding a block in the allocated block list with the same startMemBlockAddr as i_ptr
			while (tempAlloc != nullptr) {
				if (tempAlloc->startMemBlockAddr == i_ptr)
					break;
				tempAlloc = tempAlloc->nextBlock;
			}
			//the previous while loop found the correct allocated block that should be freed
			if (tempAlloc) {
				//if the chosen allocated block is at the head of the allocated blocks list
				if (tempAlloc == this->AllocatedBlocks)
				{
					//if the chosen allocated block is the head and there is more than one block in the allocated block list
					if (this->AllocatedBlocks->nextBlock != nullptr) {
                        //break all the links for this chosen allocated block from the allocated blocks list and update the links in the allocated blocks list accordingly
						this->AllocatedBlocks = this->AllocatedBlocks->nextBlock;
						this->AllocatedBlocks->prevBlock = nullptr;
						tempAlloc->prevBlock = nullptr;
						tempAlloc->nextBlock = nullptr;
					}
					//chosen allocated block is the head and is the only block in the allocated blocks list
					else {
                        //break all the links for this chosen allocated block from the allocated blocks list and update the links in the allocated blocks list accordingly
						this->AllocatedBlocks = this->AllocatedBlocks->nextBlock;
						tempAlloc->prevBlock = nullptr;
						tempAlloc->nextBlock = nullptr;
					}
					
					
				}
				//if chosen allocated block is not at the head of the allocated blocks list
				else {
					//if there exists a block before chosen allocated block
					if (tempAlloc->prevBlock != nullptr)
					{
						//if chosen allocated block has a next block
						if (tempAlloc->nextBlock != nullptr)
						{
							//set chosen allocated block's prev block's next block in allocated list to chosen allocated block's next block
							tempAlloc->prevBlock->nextBlock = tempAlloc->nextBlock;
						}
						//if chosen allocated block does not have a next block
						else 
						{
							//set chosen allocated block's prev block's next block to null since chosen allocated block has no next block in the allocated blocks list
							tempAlloc->prevBlock->nextBlock = nullptr;
						}
					}
					//if there exists a block after chosen allocated block
					if (tempAlloc->nextBlock != nullptr)
					{
						//set chosen allocated block's next block in allocated blocks list to chosen allocated block's prev block
						tempAlloc->nextBlock->prevBlock = tempAlloc->prevBlock;
					}
				}

				while (tempFree != nullptr) {
					//find the first right place in the ascending memory addresses of the free blocks list to put this newly freed tempAlloc block in (insertion sort)
					if (tempAlloc->startMemBlockAddr + tempAlloc->sizeOfBlock < reinterpret_cast<uint8_t*>(tempFree)) {
						//if tempFree is not the head of the free list
						if (tempFree->prevBlock != nullptr)
						{
                            //inserts the newly freed block in between the tempFree and it's previous block
							BlockDescriptor* oldTempFreePrevBlock = tempFree->prevBlock;
							tempFree->prevBlock = tempAlloc;
							oldTempFreePrevBlock->nextBlock = tempAlloc;
							tempAlloc->nextBlock = tempFree;
							tempAlloc->prevBlock = oldTempFreePrevBlock;
							break;
						}
						else {
                            //inserts the newly freed block at the head of the free blocks list
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
            //an allocated block with the same starting memory block address as i_ptr could not be found
			else {
				printf("Invalid free request\n");
			}
		};

		/**
        * Attempts to merge abutting free blocks
        */
		void HeapManager::Collect() {

			BlockDescriptor* tempFree = this->FreeBlocks;

			while (tempFree->nextBlock != nullptr)
			{
                // if an allocated block does not exist (based on it's starting memory block address) between two free blocks then merge 
                // those free blocks into one block and update the links in the free blocks list accordingly
				if ((tempFree->startMemBlockAddr + tempFree->sizeOfBlock) < reinterpret_cast<uint8_t*>(tempFree->nextBlock)) {
                    // if the 2 neighboring free blocks are NOT at the end of the free blocks list
					if (tempFree->nextBlock->nextBlock != nullptr)
					{
						tempFree->sizeOfBlock += tempFree->nextBlock->sizeOfBlock;
						tempFree->nextBlock->nextBlock->prevBlock = tempFree;
						tempFree->nextBlock = tempFree->nextBlock->nextBlock;
					}
                    // if the 2 neighboring free blocks are NOT at the end of the free blocks list
					else {
						tempFree->sizeOfBlock += tempFree->nextBlock->sizeOfBlock;
						tempFree->nextBlock = nullptr;
					}
				}
			}
		};

        /**
        * Returns true or false on whether a given pointer points to a memory address within the heap memory manager 
        *
        * @param i_ptr void pointer to a mmeory address.
        * @return TRUE - i_ptr points to a memory adress inside the heap memory manager. 
        *         FALSE - i_ptr does not point to a memory adress inside the heap memory manager.
        */
		bool Contains(void * i_ptr) {
			return ((i_ptr >= this->startOfHeap) && (i_ptr <= (this->startOfHeap + this->sizeOfHeap)));
		};

        /**
        * Returns true or false on whether a given pointer points to any allocated memory block address
        *
        * @param i_ptr void pointer to a mmeory address.
        * @return TRUE - i_ptr points to any allocated memory block address. 
        *         FALSE - i_ptr does not point to any allocated memory block address.
        */
		bool IsAllocated(void * i_ptr) {

			BlockDescriptor* temp = this->AllocatedBlocks;

			while (temp != nullptr) {
                // returns true if the starting memory block address of an allocated block is equal to i_ptr
				if (temp->startMemBlockAddr == reinterpret_cast<uint8_t*>(i_ptr)) {
					return 1;
				}
				temp = temp->nextBlock;
			}

			return 0;
		};

        /**
        * Gets the largest free block in size that exists in the free blocks list
        *
        * @return size of the largest block available in the free blocks list
        */
		size_t GetLargestFreeBlock() {
			
			size_t largestFreeBlock = 0;

			if (this->FreeBlocks != nullptr)
			{
				BlockDescriptor* temp = this->FreeBlocks;

				while (temp != nullptr) {
                    //compare the size of the current free block with the largest free block so far
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

        /**
        * Gets the cumulative total size of all the blocks in the free block list
        *
        * @return cumulative size of all the blocks in the free block list
        */
		size_t GetTotalFreeMemory() {

			if (this->FreeBlocks != nullptr)
			{
				size_t totalFreeMemory = 0;
				BlockDescriptor* temp = this->FreeBlocks;

				while (temp != nullptr) {
                    //adding up each memory block's actual block size (excluding block descriptor and guardbanding size)
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

        /**
        * Printing the free blocks linked list to the console showing key information
        */
		void ShowFreeBlocks() {
			BlockDescriptor* temp = this->FreeBlocks;

			while (temp != nullptr) {
				printf("FreeBlock Size: %d ", temp->sizeOfBlock);
				printf("FreeBlock Address: %d\n", temp->startMemBlockAddr);
				temp = temp->nextBlock;
			}
		};

        /**
        * Printing the allocated blocks linked list to the console showing key information
        */
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
