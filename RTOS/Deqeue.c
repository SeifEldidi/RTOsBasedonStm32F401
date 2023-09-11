/*
 * Deqeue.c
 *
 *  Created on: Sep 6, 2023
 *      Author: Seif pc
 */
#include "Deqeueu.h"

uint8_t DequeueInsertFront(DeQueue * Queue,void * Message)
{
	uint8_t Success = QUEUE_OK;
	if(!DequeueFull(Queue))
	{
		if(Queue->Front <=0)
			Queue->Front = Queue->max_elements-1;
		else
			Queue->Front -=1;
		Queue->List[Queue->Front] = Message;
		Queue->no_elements++;
	}else{
		Success = QUEUE_NOK;
	}
	return Success;
}

uint8_t DequeueInsertRear(DeQueue * Queue,void * Message)
{
	uint8_t Success = QUEUE_OK;
	if (!DequeueFull(Queue)) {
		Queue->List[Queue->Rear] = Message;
		Queue->no_elements++;
		if (Queue->Rear == Queue->max_elements - 1)
			Queue->Rear = 0;
		else
			Queue->Rear += 1;
	} else {
		Success = QUEUE_NOK;
	}
	return Success;
}

void * DequeueRemoveFront(DeQueue * Queue)
{
	void *Ret = NULL;
	if (!DequeueEmpty(Queue)) {
		Ret = Queue->List[Queue->Front];
		if (Queue->Front == Queue->max_elements -1)
			Queue->Front = 0;
		else
			Queue->Front += 1;
		Queue->no_elements--;
	} else {
		Ret = NULL;
	}
	return Ret;
}

void * DequeueRemoveRear(DeQueue * Queue)
{
	void *Ret = NULL;
	if (!DequeueEmpty(Queue)) {
		Ret = Queue->List[Queue->Rear];
		if (Queue->Front == 0)
			Queue->Front = Queue->max_elements -1;
		else
			Queue->Front -= 1;
		Queue->no_elements--;
	} else {
		Ret = NULL;
	}
	return Ret;
}

int8_t DequeueFull(DeQueue * Queue)
{
	return Queue->no_elements == Queue->max_elements;
}

int8_t DequeueEmpty(DeQueue * Queue)
{
	return Queue->no_elements == 0;
}
