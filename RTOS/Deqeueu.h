/*
 * Deqeueu.h
 *
 *  Created on: Sep 6, 2023
 *      Author: Seif pc
 */

#ifndef DEQEUEU_H_
#define DEQEUEU_H_

#include "../Inc/std_macros.h"
#include "OS.h"
#include "OSKernel.h"
#include "RTOSConfig.h"

#define QUEUE_FULL	1
#define QUEUE_EMPTY	0
#define QUEUE_OK	1
#define QUEUE_NOK	1

typedef struct
{
	int no_elements;
	int max_elements;
	void *List[OS_QUEUE_SIZE];
	uint8_t Front,Rear;
}DeQueue;

uint8_t DequeueInsertFront(DeQueue * Queue,void * Message);
uint8_t DequeueInsertRear(DeQueue * Queue,void * Message);
void * DequeueRemoveFront(DeQueue * Queue);
void * DequeueRemoveRear(DeQueue * Queue);
int8_t DequeueFull(DeQueue * Queue);
int8_t DequeueEmpty(DeQueue * Queue);

#endif /* DEQEUEU_H_ */
