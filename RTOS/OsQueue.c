/*
 * OsQueue.c
 *
 *  Created on: Sep 6, 2023
 *      Author: Seif pc
 */

#include "OsQueue.h"

extern TCB * pCurrentTask;
extern TCBLinkedList OsReadyList;
extern OsKernelControl KernelControl;
QUEUCB_t   QueueCBS[OS_QUEUE_NUMBER];

static void QueueCPYbuffer(void * Src ,void *dest,int8_t Size);
static void QueueUnblockWaiting(uint8_t QueueID);
static void QueueUnblockSending(uint8_t QueueID);

void QueueInit(uint8_t QueueID,uint8_t QueuLen,char * QueueName,uint8_t ItemSize)
{
	OSEnterCritical();
	QueueCBS[QueueID].ItemSize  = ItemSize;
	QueueCBS[QueueID].QueueName = QueueName;
	QueueCBS[QueueID].Queue.max_elements = QueuLen;
	QueueCBS[QueueID].Queue.Front =  0;
	QueueCBS[QueueID].Queue.Rear  =  0;
	QueueCBS[QueueID].Queue.no_elements = 0;
	OSExitCritical();
}

#if OS_QUEUE_SEND == TRUE

static void QueueUnblockWaiting(uint8_t QueueID)
{
	TCB *CurrentTask = QueueCBS[QueueID].OsRecievingList.Front;
	QueueCBS[QueueID].TasksWaitingRecieving--;
	OsDequeQueueElement(&(QueueCBS[QueueID].OsRecievingList),CurrentTask);
	OsInsertQueueTail(&OsReadyList, CurrentTask);
    CurrentTask->State = OS_TASK_READY;
}


QueueState_t  OsQueueSend(uint8_t QueueID,void * Message,uint8_t blocking)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if(QueueID >= 0 && QueueID < 16 )
	{
		if(blocking == OS_QUEUE_NBLOCK)
		{
			if(!DequeueFull(&QueueCBS[QueueID].Queue))
			{
				DequeueInsertRear(&QueueCBS[QueueID].Queue,Message);
			}else{
				QueueState = QueueFull;
			}
		}else if(blocking == OS_QUEUE_BLOCK)
		{
			if (!DequeueFull(&QueueCBS[QueueID].Queue))
			{
				DequeueInsertRear(&QueueCBS[QueueID].Queue,Message);
				QueueUnblockWaiting(QueueID);
			} else {
				/*-----Remove Task from Ready Queue and Insert Task into Queue Waiting List----*/
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				QueueCBS[QueueID].TasksWaitingSending++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList, NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				OsInsertQueueTail(&(QueueCBS[QueueID].OsSendingList),NewTask);
				QueueState = QueueFull;

				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsThreadYield(PRIVILEDGED_ACCESS);
			}
		}else{
			QueueState = QueueInvalidParamter;
		}
	}else{
		QueueState = QueueInvalidID;
	}
	OSExitCritical();
	return QueueState;
}
QueueState_t  OsQueueSendBack(uint8_t QueueID,void * Message,uint8_t blocking)
{
	QueueState_t QueueState = QueueOPSUCCESS;
	QueueState = OsQueueSend(QueueID,Message,blocking);
	return QueueState;
}

QueueState_t  OsQueueSendFront(uint8_t QueueID,void * Message,uint8_t blocking)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueueID >= 0 && QueueID < 16) {
		if (blocking == OS_QUEUE_NBLOCK) {
			if (!DequeueFull(&QueueCBS[QueueID].Queue)) {
				DequeueInsertFront(&QueueCBS[QueueID].Queue, Message);
			} else {
				QueueState = QueueFull;
			}
		} else if (blocking == OS_QUEUE_BLOCK) {
			if (!DequeueFull(&QueueCBS[QueueID].Queue)) {
				// A new Item is Present in the Queue Remove the first Waiting Task
				TCB *CurrentTask = QueueCBS[QueueID].OsRecievingList.Front;
				if(CurrentTask != NULL)
				{
					QueueUnblockWaiting(QueueID);
				}
				DequeueInsertFront(&QueueCBS[QueueID].Queue, Message);
			} else {
				/*-----Remove Task from Ready Queue and Insert Task into Queue Waiting List----*/
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				QueueCBS[QueueID].TasksWaitingSending++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList, NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsInsertQueueTail(&(QueueCBS[QueueID].OsSendingList), NewTask);
				OsThreadYield(PRIVILEDGED_ACCESS);
				QueueState = QueueFull;
			}
		} else {
			QueueState = QueueInvalidParamter;
		}
	} else {
		QueueState = QueueInvalidID;
	}
	OSExitCritical();
	return QueueState;
}

#if OS_QUEUE_RECIEVE == TRUE

static void QueueCPYbuffer(void * Src ,void *dest,int8_t Size)
{
	if(Src != NULL && dest != NULL)
	{
		uint8_t MemCounter = 0;
		for(MemCounter = 0; MemCounter<= Size-1 ;MemCounter++ )
			*((unsigned char*)(dest) + MemCounter)=*(((unsigned char *)Src) + MemCounter );
	}
}

static void QueueUnblockSending(uint8_t QueueID)
{
	TCB *CurrentTask = QueueCBS[QueueID].OsSendingList.Front;
	QueueCBS[QueueID].TasksWaitingSending--;
	OsDequeQueueElement(&(QueueCBS[QueueID].OsSendingList), CurrentTask);
	OsInsertQueueTail(&OsReadyList, CurrentTask);
	CurrentTask->State = OS_TASK_READY;
}

QueueState_t  OsQueueRecieve(uint8_t QueueID,void * Message,uint8_t blocking)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueueID >= 0 && QueueID < 16) {
		if (blocking == OS_QUEUE_NBLOCK)
		{
			if(!DequeueEmpty(&QueueCBS[QueueID].Queue))
			{
				void *Src = DequeueRemoveRear(&QueueCBS[QueueID].Queue);
				QueueCPYbuffer(Src,Message,QueueCBS[QueueID].ItemSize);
			}else{
				QueueState = QueueEmpty;
			}
		} else if (blocking == OS_QUEUE_BLOCK)
		{
			if (!DequeueEmpty(&QueueCBS[QueueID].Queue)) {
				if (DequeueFull(&QueueCBS[QueueID].Queue)) {
					TCB *CurrentTask = QueueCBS[QueueID].OsSendingList.Front;
					// Queue Has An empty Location
					if(CurrentTask != NULL)
						QueueUnblockSending(QueueID);
				}
				void *Src = DequeueRemoveRear(&QueueCBS[QueueID].Queue);
				QueueCPYbuffer(Src, Message, QueueCBS[QueueID].ItemSize);
			} else {
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				QueueCBS[QueueID].TasksWaitingSending++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList, NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				OsInsertQueueTail(&(QueueCBS[QueueID].OsRecievingList), NewTask);\
				/*-----Force Reschedule----*/
				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsThreadYield(PRIVILEDGED_ACCESS);
				QueueState = QueueEmpty;
			}
		} else {
			QueueState = QueueInvalidParamter;
		}
	} else {
		QueueState = QueueInvalidID;
	}
	OSExitCritical();
	return QueueState;
}

QueueState_t  OsQueueRecieveBack(uint8_t QueueID,void * Message,uint8_t blocking)
{

}

QueueState_t  OsQueueRecieveFront(uint8_t QueueID,void * Message,uint8_t blocking)
{

}

#endif

#endif
