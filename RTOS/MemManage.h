/*
 * MemManage.h
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */

#ifndef MEMMANAGE_H_
#define MEMMANAGE_H_
#include "RTOSConfig.h"
#include "../Inc/std_macros.h"
/*----------To Align Memory Blocks on 4bytes----*/
#define HEADER_SIZE 		8
#define HEADER_SHIFT		3
#define SIZE_FIELD			3
#define SIZE_MSK			(0xFFFFFFF8)
#define MEMORY_EMPTY		0

#define MEMORY_ALLOCATED	(1<<0)
#define MEMORY_FREE			(0<<0)

typedef struct header
{
	struct header * NextFree;
	uint32_t Size;
}Header_t;


void   OsMallocInit();
void * OsMalloc(unsigned int NoBytes);
void   OsFree(void *Free);

#endif /* MEMMANAGE_H_ */
