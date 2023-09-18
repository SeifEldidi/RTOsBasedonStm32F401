/*
 * EventGroups.h
 *
 *  Created on: Sep 18, 2023
 *      Author: Seif pc
 */

#ifndef EVENTGROUPS_H_
#define EVENTGROUPS_H_

#include "OS.h"
#include "MemManage.h"
#include "QueueControl.h"
#include "OSKernel.h"

#define EVENT_WAIT_AND	1
#define EVENT_WAIT_OR	0

#define OS_EVENT_BLOCK		1
#define OS_EVENT_NBLOCK		0

typedef struct
{
	void * EventName;
	EventType EventGroupBits;
	TCBLinkedList EventGroupWaitingList;
}EventGroup_t;

typedef EventGroup_t EventGroup;
typedef EventGroup_t * pEventGroup;

#if OS_SCHEDULER_STATIC == FALSE
pEventGroup      EventGroupCreate    (void * Name);
EventType		 EventGroupWaitBits (pEventGroup Event,EventType MSK,uint8_t EventWaitFlag,uint8_t Blocking_Nblocking);
EventType		 EventGroupSetBits  (pEventGroup Event,EventType MSK);
EventType		 EventGroupClearBits(pEventGroup Event,EventType MSK);
EventType		 EventGroupGetBits  (pEventGroup Event,EventType MSK);
#elif OS_SCHEDULER_STATIC == TRUE
void             EventGroupCreate   (void * Name , uint8_t EventID);
EventType		 EventGroupWaitBits (uint8_t EventID,EventType MSK,uint8_t EventWaitFlag,uint8_t Blocking_Nblocking);
EventType		 EventGroupSetBits  (uint8_t EventID,EventType MSK);
EventType		 EventGroupClearBits(uint8_t EventID,EventType MSK);
EventType		 EventGroupGetBits  (uint8_t EventID,EventType MSK);
#endif

#endif /* EVENTGROUPS_H_ */
