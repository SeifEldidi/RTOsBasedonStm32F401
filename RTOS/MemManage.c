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

void   OsMallocInit()
{
	BaseList = (Header_t *)(&OsHeap[0]);
	BaseList->Size 	   = (((char*)&OsHeap[OS_HEAP_SIZE] - (char*)&OsHeap[0])>>HEADER_SHIFT);
	BaseList->NextFree = BaseList;
}

void * OsMalloc(unsigned int NoBytes)
{
	Header_t *Prev = NULL;
	Header_t *Curr = NULL;
	int       Nuints = 0;
	void *    Ptr = NULL;
	Nuints = ((NoBytes+HEADER_SIZE-1)>>HEADER_SHIFT) + 1;

	// init List if not initiliazed
	//Allocate 1024 Byte for heap and Set Header to point to next free Block or NULL
	if(BaseList == NULL)
		OsMallocInit();

	Prev = BaseList;
	Curr = BaseList->NextFree;

	while(Curr != NULL)
	{
		if(Curr->Size >= Nuints)
		{
			if(Curr->Size == Nuints)
			{
				//Block is the Exact Size
				//point to Next Block Error Will Occur if 1 Block Remains
				Prev->NextFree = Curr->NextFree;
			}else{
				Curr->Size -= Nuints;
				// Remove Block from the end of the Current Block
				Curr += Curr->Size;
				Curr->Size = Nuints;
			}
			BaseList = Prev;
			Ptr = (void *)(Curr + 1);
			break;
		}
		Prev = Curr;
		Curr = Curr->NextFree;
	}
	return Ptr;
}

void   OsFree(void *Free)
{
	Header_t *insertp = NULL;
	Header_t *currp = BaseList;

	insertp = ((Header_t *)Free) - 1;
	/*--------Traverse List------*/
	while((currp != NULL )&&(currp <insertp && insertp<currp->NextFree))
	{
		if((currp >= currp->NextFree) && ((currp < insertp) || (insertp < currp->NextFree)))
				break;
		currp = currp->NextFree;
	}
	if ((insertp + insertp->Size) == currp->NextFree) {
		insertp->Size    += currp->NextFree->Size;
		insertp->NextFree = currp->NextFree->NextFree;
	}
	else {
		insertp->NextFree = currp->NextFree;
	}

	if ((currp + currp->Size) == insertp) {
		currp->Size += insertp->Size;
		currp->NextFree = insertp->NextFree;
	}
	else {
		currp->NextFree = insertp;
	}

	BaseList = currp;
}

