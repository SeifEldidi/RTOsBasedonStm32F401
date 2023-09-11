/*
 * OsQueueControl.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "QueueControl.h"

void OsInsertQueueHead(TCBLinkedList * Queue,TCB *Elem)
{
	if(Elem != NULL)
	{
		TCB * NewTask = Elem;
		if(Queue->Front == NULL)
		{
			Queue->Front = Queue->Rear = NewTask;
			Queue->Front->Next_Task = NULL;
		}else{
			NewTask->Next_Task = Queue->Front;
			Queue->Front = NewTask;
		}
		Queue->No_Tasks++;
	}
}

void OsInsertQueueSorted(TCBLinkedList * Queue,TCB *Elem)
{
	if (Elem != NULL) {
		TCB *PrevTask = NULL;
		TCB *CurrTask = Queue->Front;
		while(CurrTask&&CurrTask->Priority>=Elem->Priority)
		{
			PrevTask = CurrTask;
			CurrTask = CurrTask->Next_Task;
		}
		if(PrevTask != NULL)
		{
			Elem->Next_Task = CurrTask;
			PrevTask->Next_Task = Elem;
			if(CurrTask == NULL)
				Queue->Rear = Elem;
			else{}
		}else{
			Queue->Front = Elem;
			Elem->Next_Task = CurrTask;
			if(CurrTask == NULL)
			{
				Queue->Rear = Elem;
			}else{}
		}
		Queue->No_Tasks++;
	}
}

void OsInsertQueueTail(TCBLinkedList * Queue,TCB *Elem)
{
	if (Elem != NULL) {
		TCB *NewTask = Elem;
		if (Queue->Front == NULL) {
			Queue->Front = Queue->Rear = NewTask;
			Queue->Front->Next_Task = NULL;
		} else {
			Queue->Rear->Next_Task = NewTask;
			Queue->Rear = Queue->Rear->Next_Task;
			Queue->Rear->Next_Task = NULL;
		}
		Elem->Next_Task = NULL;
		Queue->No_Tasks++;
	}
}

TCB *  OsDequeQueueElement(TCBLinkedList * Queue,TCB *Elem)
{
	TCB * Prev = NULL;
	TCB * Head = Queue->Front;
	/*-----Search For Previous----*/
	while(Head && Head!=Elem )
	{
		Prev = Head;
		Head=Head->Next_Task;
	}
	if(Head!= NULL && Prev != NULL)
	{
		if(Elem == Queue->Rear)
		{
			Prev->Next_Task = NULL;
			Queue->Rear=Prev;
		}else{
			Prev->Next_Task = Elem->Next_Task;
		}
		Elem->Next_Task = NULL;
		Queue->No_Tasks --;
	}else if(Head!= NULL && Prev == NULL){
		if(Queue->Front == Queue->Rear)
		{
			Queue->Front = Queue->Front->Next_Task;
			Queue->Rear = Queue->Front;
		}else
			Queue->Front = Queue->Front->Next_Task;
		Elem->Next_Task = NULL;
		Queue->No_Tasks --;
	}
	return Head;
}

TCB* OsDequeQueueRear(TCBLinkedList * Queue)
{
	TCB * QueueRear = NULL;
	if(Queue != NULL)
	{
		if(Queue->No_Tasks >0)
		{
			QueueRear = Queue->Rear->Next_Task;
			Queue->Rear->Next_Task = NULL;
			QueueRear->Next_Task = NULL;
			Queue->No_Tasks--;
		}else{
			QueueRear = NULL;
		}
	}else{
		QueueRear = NULL;
	}
	return QueueRear;
}

TCB* OsDequeQueueFront(TCBLinkedList * Queue)
{
	TCB *QueueFront= NULL;
	if (Queue != NULL) {
		if (Queue->No_Tasks > 0) {
			QueueFront   = Queue->Front;
			Queue->Front = Queue->Front->Next_Task;
			QueueFront->Next_Task = NULL;
			if(Queue->No_Tasks == 1)
				Queue->Rear = Queue->Front;
			Queue->No_Tasks--;
		} else {
			QueueFront = NULL;
		}
	} else {
		QueueFront = NULL;
	}
	return QueueFront;
}
