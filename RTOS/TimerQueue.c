/*
 * TimerQueue.c
 *
 *  Created on: Sep 18, 2023
 *      Author: Seif pc
 */
#include "TimerQueue.h"

void OsTimerQueueInsertHead(OsTimerDLL * List, OsTimer_t *Elem)
{
	if(Elem != NULL)
	{
		/*-------Insert New Element----*/
		if(List->No_Timers == 0)
		{
			List->Head = Elem;
			List->Tail = List->Head;
			Elem->Prev = NULL;
		}else{
			Elem->Next = List->Head;
			List->Head->Prev = Elem;
			List->Head = Elem;
		}
		List->No_Timers++;
	}
}

void OsTimerQueueInsertTail(OsTimerDLL * List, OsTimer_t *Elem)
{
	if (Elem != NULL) {
		if(List->No_Timers == 0)
		{
			Elem->Prev = NULL;
			List->Head = Elem;
			List->Tail = List->Head;
		}else{
			List->Tail->Next = Elem;
			Elem->Next = NULL;
			Elem->Prev = List->Tail;
			List->Tail = List->Tail->Next;
		}
		List->No_Timers++;
	}
}

OsTimer_t *OsTimerDequeQueueElement(OsTimerDLL * List,OsTimer_t *Elem)
{
	OsTimer_t * RetPtr = NULL;
	if(Elem != NULL && List != NULL)
	{
		if(List->No_Timers == 0)
		{
			RetPtr = NULL;
		}else{
			if(List->Head == Elem)
			{
				OsTimerDequeQueueFront(List);
			}else if(List->Tail == Elem)
			{
				OsTimerDequeQueueRear(List);
			}else{
				RetPtr = Elem;
				OsTimer_t *Prev = Elem->Prev;
				OsTimer_t *Next = Elem->Next;

				Elem->Next = NULL;
				Elem->Prev = NULL;
				Prev->Next = Next;
				if (Next != NULL)
					Next->Prev = Prev;
				else {
				}
				List->No_Timers--;
			}
		}
	}else{
		RetPtr = NULL;
	}
	return RetPtr;
}

OsTimer_t *OsTimerDequeQueueRear(OsTimerDLL * List)
{
	OsTimer_t *QueueRear = NULL;
	if(List != NULL)
	{
		if(List->No_Timers > 0)
		{
			if(List->No_Timers == 1)
			{
				QueueRear = List->Head;
				List->Head = NULL;
				List->Tail = NULL;
			}else{
				OsTimer_t * Prev = List->Tail->Prev;
				OsTimer_t * Next = List->Tail->Next;
				QueueRear = List->Tail;

				QueueRear->Prev = NULL;
				QueueRear->Next = NULL;
				Prev->Next = Next;
				if(Next != NULL)
				{
					Next->Prev = Prev;
				}else{

				}
			}
			List->No_Timers--;
		}else{
			QueueRear = NULL;
		}
	}else{
		QueueRear = NULL;
	}
	return QueueRear;
}

OsTimer_t *OsTimerDequeQueueFront(OsTimerDLL * List)
{
	OsTimer_t * QueueFront= NULL;
	if (List != NULL) {
		if (List->No_Timers > 0)
		{
			if (List->No_Timers == 1)
			{
				QueueFront = List->Head;
				List->Head = NULL;
				List->Tail = NULL;
			} else
			{
				OsTimer_t * Next = List->Head->Next;
				QueueFront = List->Head;
				QueueFront->Next = NULL;
				Next->Prev = NULL;
				List->Head = Next;
			}
			List->No_Timers--;
		} else {
			QueueFront = NULL;
		}
	} else {
		QueueFront = NULL;
	}
	return QueueFront;
}
