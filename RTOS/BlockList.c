/*
 * BlockList.c
 *
 *  Created on: Sep 14, 2023
 *      Author: Seif pc
 */

#include "BlockList.h"


void InsertBlockList(BlockDLL *List,Block *Elem)
{
	if (Elem != NULL) {
		if (List->No_Blocks == 0) {
			Elem->Prev = NULL;
			List->Head = Elem;
			List->Tail = List->Head;
			List->No_Blocks++;
		} else {
			uint8_t Priority = Elem->PriorityVal;
			Block *Iterator = List->Head;
			Block *PrevIterator = NULL;
			while(Iterator->PriorityVal != Priority)
			{
				PrevIterator = Iterator;
				Iterator = Iterator->Next;
			}
			if(Iterator == NULL)
			{
				List->No_Blocks++;
				PrevIterator->Next = Elem;
				Elem->Prev = PrevIterator;
			}else{
				Iterator->pTCB = Elem->pTCB;
				Iterator->PriorityVal = Elem->PriorityVal;
			}
		}
	} else {

	}
}

void RemoveBlockListHead(BlockDLL *List)
{
	Block *ListFront = NULL;
	if (List != NULL) {
		if (List->No_Blocks > 0) {
			if (List->No_Blocks == 1) {
				ListFront = List->Head;
				List->Head = NULL;
				List->Tail = NULL;
			} else {
				Block *Next = List->Head->Next;
				ListFront = List->Head;
				ListFront->Next = NULL;
				List->Head = Next;
			}
			List->No_Blocks--;
		} else {
			ListFront = NULL;
		}
	} else {

	}
}
void RemoveBlockListTail(BlockDLL *List)
{
	Block *ListRear = NULL;
	if (List != NULL) {
		if (List->No_Blocks > 0) {
			if (List->No_Blocks == 1) {
				ListRear = List->Head;
				List->Head = NULL;
				List->Tail = NULL;
			} else {
				Block *Prev = List->Tail->Prev;
				Block *Next = List->Tail->Next;
				ListRear = List->Tail;

				ListRear->Next = NULL;
				ListRear->Prev = NULL;
				Prev->Next = Next;
				if (Next != NULL) {
					Next->Next = Prev;
				} else {

				}
			}
			List->No_Blocks--;
		} else {
			ListRear = NULL;
		}
	} else {
		ListRear = NULL;
	}
}
void RemoveBlockListElem(BlockDLL *List,Block *Elem)
{
	if (Elem != NULL && List != NULL) {
		if (List->No_Blocks == 0) {

		} else {
			if (List->Head == Elem) {
				RemoveBlockListHead(List);
			} else if (List->Tail == Elem) {
				RemoveBlockListTail(List);
			} else {
				Block *Prev = Elem->Prev;
				Block *Next = Elem->Next;

				Elem->Next = NULL;
				Elem->Prev = NULL;
				Prev->Next = Next;
				if (Next != NULL)
					Next->Prev = Prev;
				else {
				}
				List->No_Blocks--;
			}
		}
	} else {
	}
}
