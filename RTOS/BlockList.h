/*
 * BlockList.h
 *
 *  Created on: Sep 14, 2023
 *      Author: Seif pc
 */

#ifndef BLOCKLIST_H_
#define BLOCKLIST_H_

#include "../Inc/std_macros.h"

typedef struct
{
	void *Next;
	void *Prev;
	void *pTCB;
	uint8_t PriorityVal;
}Block;

typedef struct
{
	Block *Head,*Tail;
	int32_t No_Blocks;
}BlockDLL;

void InsertBlockList(BlockDLL *List,Block *Elem);

void RemoveBlockListHead(BlockDLL *List);
void RemoveBlockListTail(BlockDLL *List);
void RemoveBlockListElem(BlockDLL *List,Block *Elem);

#endif /* BLOCKLIST_H_ */
