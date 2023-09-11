/*
 * MemManage.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "MemManage.h"

// One Big Block of size 1024Bye---*/
uint8_t OsHeap[OS_HEAP_SIZE];
int8_t PartitionNumber = 0;

Header_t* BaseList    = NULL;
uint32_t  Bytes_left  = 0;

void   OsMallocInit(Header_t **Ref)
{
	BaseList = (Header_t *)(&OsHeap[0]);
	BaseList->Size 	   = (((char*)&OsHeap[OS_HEAP_SIZE] - (char*)&OsHeap[0])>>HEADER_SHIFT);
	BaseList->NextFree = NULL;
	Bytes_left = BaseList->Size;
	*Ref = BaseList;
}

void * OsMalloc(unsigned int NoBytes)
{
	Header_t *Prev = NULL;
	Header_t *Curr = BaseList;
	int       Nuints = 0;
	void *    Ptr = NULL;
	Nuints = ((NoBytes+HEADER_SIZE-1)>>HEADER_SHIFT) + 1;

	// init List if not initiliazed
	//Allocate 1024 Byte for heap and Set Header to point to next free Block or NULL
	if(BaseList == NULL)
		OsMallocInit(&Curr);

	if(Bytes_left > 0)
	{
		while(Curr != NULL)
		{
			if(Curr->Size >= Nuints)
			{
				if(Curr->Size == Nuints)
				{
					if(Prev != NULL)
						Prev->NextFree = Curr->NextFree;
				}else{
					Curr->Size -= Nuints;
					// Remove Block from the end of the Current Block
					Curr += Curr->Size;
					Curr->Size = Nuints;
					Bytes_left -= Nuints;
				}
				Ptr = (void *)(Curr + 1);
				break;
			}
			Prev = Curr;
			Curr = Curr->NextFree;
		}
	}else{
		Ptr = NULL;
	}
	return Ptr;
}

void   OsFree(void *Free)
{
	Header_t * Current = BaseList;

	Header_t * FreeNode = (((Header_t *) Free) - 1);
	/*-----Search Free List for Block Position----*/
	while (Current != NULL) {
		if (Current < FreeNode
				&& (Current->NextFree > FreeNode || Current->NextFree == NULL))
			break;
		Current = Current->NextFree;
	}
	if (FreeNode + FreeNode->Size == Current->NextFree) {
		FreeNode->NextFree = Current->NextFree->NextFree;
		FreeNode->Size += Current->NextFree->Size;
		Current->NextFree->NextFree = NULL;
		Current->NextFree->Size = 0;
	} else {
		FreeNode->NextFree = Current->NextFree;
	}
	if (Current + Current->Size == FreeNode) {
		Current->Size += FreeNode->Size;
		Current->NextFree = FreeNode->NextFree;
		FreeNode->NextFree = NULL;
		FreeNode->Size = 0;
	} else {
		Current->NextFree = FreeNode;
	}
}

