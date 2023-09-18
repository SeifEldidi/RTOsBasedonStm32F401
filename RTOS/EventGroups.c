/*
 * EventGroups.c
 *
 *  Created on: Sep 18, 2023
 *      Author: Seif pc
 */
#include "EventGroups.h"

extern TCB * pCurrentTask;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
extern TCBLinkedList OsReadyList;
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
extern TCBLinkedList OsReadyFIFO[OS_MAX_PRIORITY];
#endif
extern OsKernelControl KernelControl;

static void EventBlockTask(pEventGroup Event,TCB *CurrentTask)
{
#if  OS_SCHEDULER_STATIC == FALSE
	CurrentTask->State = OS_TASK_BLOCKED;
	CurrentTask->CurrQueue = (void *)&Event->EventGroupWaitingList;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	OsInsertQueueSorted(&Event->EventGroupWaitingList,CurrentTask);
#elif OS_SCHEDULER_PRIORITY == OS_SCHEDULER_ROUND_ROBIN
	OsDequeQueueElement(&OsReadyList,CurrentTask);
	OsInsertQueueTail(&Event->EventGroupWaitingList,CurrentTask);
#endif
#elif OS_SCHEDULER_STATIC == TRUE
	CurrentTask->State = OS_TASK_BLOCKED;
	CurrentTask->CurrQueue = (void *)&Event->EventGroupWaitingList;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	OsInsertQueueSorted(&Event->EventGroupWaitingList,CurrentTask);
#elif OS_SCHEDULER_PRIORITY == OS_SCHEDULER_ROUND_ROBIN
	OsDequeQueueElement(&OsReadyList,CurrentTask);
	OsInsertQueueTail(&Event->EventGroupWaitingList,CurrentTask);
#endif
#endif
}

#if OS_SCHEDULER_STATIC == FALSE
pEventGroup EventGroupCreate   (void * Name)
{
	OSEnterCritical();
	pEventGroup xEventGroup = (pEventGroup)OsMalloc(sizeof(EventGroup));
	if(xEventGroup != NULL)
	{
		xEventGroup->EventGroupBits = 0;
		xEventGroup->EventGroupWaitingList.Front    = NULL;
		xEventGroup->EventGroupWaitingList.No_Tasks = 0;
		xEventGroup->EventGroupWaitingList.Rear 	= NULL;
	}else{
		xEventGroup = NULL;
	}
	OSExitCritical();
 	return xEventGroup;
}

EventType  EventGroupWaitBits (pEventGroup Event,EventType MSK,uint8_t EventWaitFlag,uint8_t Blocking_Nblocking)
{
	OSEnterCritical();
	EventType Eventbits = Event->EventGroupBits;
	pCurrentTask->EventFlag = MSK ;
	pCurrentTask->EventWait = EventWaitFlag ;
	if(Event != NULL)
	{
		Eventbits = Event->EventGroupBits;
		if(EventWaitFlag == EVENT_WAIT_AND)
		{
			if((Event->EventGroupBits & MSK )== MSK)
			{
				Eventbits = Event->EventGroupBits;
			}else{
				//Block Task
				if(Blocking_Nblocking == OS_EVENT_BLOCK )
				{
					EventBlockTask(Event , pCurrentTask);
					KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
					OsThreadYield(PRIVILEDGED_ACCESS);
				}
				else{}
			}
		}else if(EventWaitFlag == EVENT_WAIT_OR)
		{
			if((Event->EventGroupBits & MSK )!= 0)
			{
				Eventbits = Event->EventGroupBits;
			}else{
				//Block Task
				if (Blocking_Nblocking == OS_EVENT_BLOCK)
				{
					EventBlockTask(Event, pCurrentTask);
					KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
					OsThreadYield(PRIVILEDGED_ACCESS);
				}
				else{}
			}
		}else{
			if (Blocking_Nblocking == OS_EVENT_BLOCK)
			{
				EventBlockTask(Event, pCurrentTask);
				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsThreadYield(PRIVILEDGED_ACCESS);
			}
			else{}
		}
	}else{

	}
	OSExitCritical();
	return Eventbits;
}

EventType  EventGroupSetBits  (pEventGroup Event,EventType MSK)
{
	OSEnterCritical();
	EventType Eventbits = Event->EventGroupBits;
	if (Event != NULL) {
		uint8_t Priority       = pCurrentTask->Priority;
		uint8_t ContextFlag    = 0;
		Event->EventGroupBits |= MSK;
		//does this Setting of Bits unblock a Task Lets check
		TCB *head = Event->EventGroupWaitingList.Front;
		//Check
		while(head != NULL)
		{
			if(head->EventWait == EVENT_WAIT_AND)
			{
				if((head->EventFlag & MSK ) == MSK)
				{
					//Remove from Event Waiting List
					OsDequeQueueElement(&Event->EventGroupWaitingList,head);
					//Insert into Ready FIFO
					OsInsertQueueTail(&OsReadyFIFO[head->Priority],head);
				}else{
				}
			}else if(head->EventWait == EVENT_WAIT_OR){
				if((head->EventFlag & MSK ) != 0)
				{
					//Remove from Event Waiting List
					OsDequeQueueElement(&Event->EventGroupWaitingList,head);
					//Insert into Ready FIFO
					OsInsertQueueTail(&OsReadyFIFO[head->Priority],head);
				}else{

				}
			}else{

			}
			if(head->Priority > Priority)
				ContextFlag = 1;
			head = head->Next_Task;
		}
		if(ContextFlag == 1)
		{
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			OsThreadYield(PRIVILEDGED_ACCESS);
		}
	} else {

	}
	OSExitCritical();
	return Eventbits;
}

EventType  EventGroupClearBits(pEventGroup Event,EventType MSK)
{
	OSEnterCritical();
	EventType Eventbits = 0;
	if (Event != NULL) {
		Event->EventGroupBits = 0;
	} else {
		Eventbits = 0;
	}
	OSExitCritical();
	return Eventbits;
}

EventType  EventGroupGetBits  (pEventGroup Event,EventType MSK)
{
	OSEnterCritical();
	EventType Eventbits = 0;
	if (Event != NULL) {
		Eventbits = Event->EventGroupBits;
	} else {
		Eventbits = 0;
	}
	OSExitCritical();
	return Eventbits;
}

#elif OS_SCHEDULER_STATIC == TRUE
void         EventGroupCreate   (void * Name , uint8_t EventID);
EventType		 EventGroupWaitBits (uint8_t EventID,EventType MSK,uint8_t EventWaitFlag,uint8_t Blocking_Nblocking);
EventType		 EventGroupSetBits  (uint8_t EventID,EventType MSK);
EventType		 EventGroupClearBits(uint8_t EventID,EventType MSK);
EventType		 EventGroupGetBits  (uint8_t EventID,EventType MSK);
#endif
