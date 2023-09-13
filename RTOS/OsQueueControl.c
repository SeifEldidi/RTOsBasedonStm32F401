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
		/*-------Insert New Element----*/
		if(Queue->No_Tasks == 0)
		{
			Queue->Front = Elem;
			Queue->Rear  = Queue->Front;
			Elem->Prev_Task = NULL;
		}else{
			Elem->Next_Task = Queue->Front;
			Queue->Front->Prev_Task = Elem;
			Queue->Front = Elem;
		}
		Queue->No_Tasks++;
	}
}

void OsInsertQueueSorted(TCBLinkedList * Queue,TCB *Elem)
{
	if (Elem != NULL) {
		if(Queue->No_Tasks == 0)
		{
			Elem->Prev_Task = NULL;
			Queue->Front = Elem;
			Queue->Rear  = Queue->Front;
		}else{
			TCB *Iterator    = Queue->Front;
			TCB *PrevIterator = Iterator->Prev_Task;
			while (Iterator->Priority >= Elem->Priority)
			{
				PrevIterator = Iterator;
				Iterator = Iterator->Next_Task;
			}
			if(PrevIterator != NULL)
			{
				TCB * Next = PrevIterator->Next_Task;
				if(Next != NULL)
				{
					Elem->Prev_Task = Next->Prev_Task;
					Next->Prev_Task = Elem;
				}else{
					Elem->Prev_Task = PrevIterator;
				}
				Elem->Next_Task = Next;
				PrevIterator->Next_Task = Elem;

				if(PrevIterator == Queue->Rear)
					Queue->Rear = Elem;
			}else{
				Elem->Next_Task = Iterator;
				Iterator->Prev_Task = Elem;
				Elem->Prev_Task = NULL;
			}
		}
		Queue->No_Tasks++;
	}else{

	}
}

void OsInsertQueueTail(TCBLinkedList * Queue,TCB *Elem)
{
	if (Elem != NULL) {
		if(Queue->No_Tasks == 0)
		{
			Elem->Prev_Task = NULL;
			Queue->Front = Elem;
			Queue->Rear = Queue->Front;
		}else{
			Queue->Rear->Next_Task = Elem;
			Elem->Next_Task = NULL;
			Elem->Prev_Task = Queue->Rear;
			Queue->Rear = Queue->Rear->Next_Task;
		}
		Queue->No_Tasks++;
	}
}

TCB *  OsDequeQueueElement(TCBLinkedList * Queue,TCB *Elem)
{
	TCB * RetPtr = NULL;
	if(Elem != NULL && Queue != NULL)
	{
		if(Queue->No_Tasks == 0)
		{
			RetPtr = NULL;
		}else{
			if(Queue->Front == Elem)
			{
				OsDequeQueueFront(Queue);
			}else if(Queue->Rear == Elem)
			{
				OsDequeQueueRear(Queue);
			}else{
				RetPtr = Elem;
				TCB *Prev = Elem->Prev_Task;
				TCB *Next = Elem->Next_Task;

				Elem->Next_Task = NULL;
				Elem->Prev_Task = NULL;
				Prev->Next_Task = Next;
				if (Next != NULL)
					Next->Prev_Task = Prev;
				else {
				}
				Queue->No_Tasks--;
			}
		}
	}else{
		RetPtr = NULL;
	}
	return RetPtr;
}

TCB* OsDequeQueueRear(TCBLinkedList * Queue)
{
	TCB * QueueRear = NULL;
	if(Queue != NULL)
	{
		if(Queue->No_Tasks > 0)
		{
			if(Queue->No_Tasks == 1)
			{
				QueueRear = Queue->Front;
				Queue->Front = NULL;
				Queue->Rear = NULL;
			}else{
				TCB *Prev = Queue->Rear->Prev_Task;
				TCB *Next = Queue->Rear->Next_Task;
				QueueRear = Queue->Rear;

				QueueRear->Next_Task = NULL;
				QueueRear->Prev_Task = NULL;
				Prev->Next_Task = Next;
				if(Next != NULL)
				{
					Next->Prev_Task = Prev;
				}else{

				}
			}
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
		if (Queue->No_Tasks > 0)
		{
			if (Queue->No_Tasks == 1)
			{
				QueueFront = Queue->Front;
				Queue->Front = NULL;
				Queue->Rear = NULL;
			} else
			{
				TCB *Next = Queue->Front->Next_Task;
				QueueFront = Queue->Front;
				QueueFront->Next_Task = NULL;
				Queue->Front = Next;
			}
			Queue->No_Tasks--;
		} else {
			QueueFront = NULL;
		}
	} else {
		QueueFront = NULL;
	}
	return QueueFront;
}
