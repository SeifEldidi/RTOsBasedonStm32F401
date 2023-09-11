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

#if OS_QUEUE_NUMBER >=0 && OS_QUEUE_NUMBER<=16
QUEUCB_t   QueueCBS[OS_QUEUE_NUMBER];
#if OS_SCHEDULER_STATIC == TRUE
static void QueueUnblockWaiting(uint8_t QueueID);
static void QueueUnblockSending(uint8_t QueueID);
#elif OS_SCHEDULER_STATIC == FALSE
static void QueueUnblockSending(pQueue QueueID);
static void QueueUnblockWaiting(pQueue QueuePtr);
#endif
static void QueueCPYbuffer(void * Src ,void *dest,int8_t Size);

void QueueCreateStatic(uint8_t QueueID,uint8_t QueuLen,char * QueueName,uint8_t ItemSize)
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

pQueue QueueCreateDynamic(uint8_t QueuLen,char * QueueName,uint8_t ItemSize)
{
	OSEnterCritical();
	pQueue PQueue = (pQueue)OsMalloc(1*sizeof(Queue));
	if(PQueue != NULL)
	{
		PQueue->ItemSize  = ItemSize;
		PQueue->QueueName = QueueName;
		PQueue->Queue.List = (void **)OsMalloc(sizeof(void *)*QueuLen);
		if(PQueue->Queue.List != NULL)
		{
			PQueue->Queue.Front 	= 0;
			PQueue->Queue.Rear 		= 0;
			PQueue->Queue.max_elements = QueuLen;
			PQueue->Queue.no_elements = 0;
		}else{
			OsFree(PQueue);
			PQueue = NULL;
		}
	}else{
		PQueue = NULL;
	}
	OSExitCritical();
	return PQueue;
}

#if OS_QUEUE_SEND == TRUE

#if OS_SCHEDULER_STATIC == TRUE
static void QueueUnblockWaiting(uint8_t QueueID)
{
	TCB *CurrentTask = QueueCBS[QueueID].OsRecievingList.Front;
	QueueCBS[QueueID].TasksWaitingRecieving--;
	OsDequeQueueElement(&(QueueCBS[QueueID].OsRecievingList),CurrentTask);
	OsInsertQueueTail(&OsReadyList, CurrentTask);
    CurrentTask->State = OS_TASK_READY;
}
#elif OS_SCHEDULER_STATIC == FALSE
static void QueueUnblockWaiting(pQueue QueuePtr)
{
	TCB *CurrentTask = QueuePtr->OsRecievingList.Front;
	QueuePtr->TasksWaitingRecieving--;
	OsDequeQueueElement(&(QueuePtr->OsRecievingList),CurrentTask);
	OsInsertQueueTail(&OsReadyList, CurrentTask);
    CurrentTask->State = OS_TASK_READY;
}
#endif

#if OS_SCHEDULER_STATIC == TRUE
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

QueueState_t  OsQueueSendIsr(uint8_t QueueID,void * Message)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueueID >= 0 && QueueID < 16) {
		if (!DequeueFull(&QueueCBS[QueueID].Queue))
		{
			DequeueInsertRear(&QueueCBS[QueueID].Queue, Message);
		} else {
			QueueState = QueueFull;
		}
	}
	OSExitCritical();
	return QueueState;
}
#elif OS_SCHEDULER_STATIC == FALSE
QueueState_t  OsQueueSend(pQueue QueuePtr,void * Message,uint8_t blocking)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if(QueuePtr != NULL)
	{
		if(blocking == OS_QUEUE_NBLOCK)
		{
			if(!DequeueFull(&QueuePtr->Queue))
			{
				DequeueInsertRear(&QueuePtr->Queue,Message);
			}else{
				QueueState = QueueFull;
			}
		}else if(blocking == OS_QUEUE_BLOCK)
		{
			if (!DequeueFull(&QueuePtr->Queue))
			{
				DequeueInsertRear(&QueuePtr->Queue,Message);
				QueueUnblockWaiting(QueuePtr);
			} else {
				/*-----Remove Task from Ready Queue and Insert Task into Queue Waiting List----*/
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				QueuePtr->TasksWaitingSending++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList, NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				OsInsertQueueTail(&(QueuePtr->OsSendingList),NewTask);
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

QueueState_t  OsQueueSendIsr(pQueue QueuePtr,void * Message)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueuePtr != NULL) {
		if (!DequeueFull(&QueuePtr->Queue)) {
			DequeueInsertRear(&QueuePtr->Queue, Message);
		} else {
			QueueState = QueueFull;
		}
	}
	OSExitCritical();
	return QueueState;
}

#endif
#if OS_SCHEDULER_STATIC == FALSE
QueueState_t  OsQueueSendBack(pQueue QueuePtr,void * Message,uint8_t blocking)
{
	QueueState_t QueueState = QueueOPSUCCESS;
	QueueState = OsQueueSend(QueuePtr,Message,blocking);
	return QueueState;
}
#elif OS_SCHEDULER_STATIC == TRUE
QueueState_t  OsQueueSendBack(uint8_t QueueID,void * Message,uint8_t blocking)
{
	QueueState_t QueueState = QueueOPSUCCESS;
	QueueState = OsQueueSend(QueueID,Message,blocking);
	return QueueState;
}
#endif

#if OS_SCHEDULER_STATIC == TRUE
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
#elif OS_SCHEDULER_STATIC == FALSE
QueueState_t  OsQueueSendFront(pQueue QueuePtr,void * Message,uint8_t blocking)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueuePtr != NULL) {
		if (blocking == OS_QUEUE_NBLOCK) {
			if (!DequeueFull(&QueuePtr->Queue)) {
				DequeueInsertFront(&QueuePtr->Queue, Message);
			} else {
				QueueState = QueueFull;
			}
		} else if (blocking == OS_QUEUE_BLOCK) {
			if (!DequeueFull(&QueuePtr->Queue)) {
				// A new Item is Present in the Queue Remove the first Waiting Task
				TCB *CurrentTask = QueuePtr->OsRecievingList.Front;
				if(CurrentTask != NULL)
				{
					QueueUnblockWaiting(QueuePtr);
				}
				DequeueInsertFront(&QueuePtr->Queue, Message);
			} else {
				/*-----Remove Task from Ready Queue and Insert Task into Queue Waiting List----*/
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				QueuePtr->TasksWaitingSending++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList, NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsInsertQueueTail(&(QueuePtr->OsSendingList), NewTask);
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
#endif

#if OS_QUEUE_RECIEVE == TRUE

#if OS_SCHEDULER_STATIC == TRUE
static void QueueUnblockSending(uint8_t QueueID)
{
	TCB *CurrentTask = QueueCBS[QueueID].OsSendingList.Front;
	QueueCBS[QueueID].TasksWaitingSending--;
	OsDequeQueueElement(&(QueueCBS[QueueID].OsSendingList), CurrentTask);
	OsInsertQueueTail(&OsReadyList, CurrentTask);
	CurrentTask->State = OS_TASK_READY;
}

#elif OS_SCHEDULER_STATIC == FALSE
static void QueueUnblockSending(pQueue QueuePtr)
{
	TCB *CurrentTask = QueuePtr->OsSendingList.Front;
	QueuePtr->TasksWaitingSending--;
	OsDequeQueueElement(&(QueuePtr->OsSendingList), CurrentTask);
	OsInsertQueueTail(&OsReadyList, CurrentTask);
	CurrentTask->State = OS_TASK_READY;
}
#endif

#if OS_SCHEDULER_STATIC == TRUE
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

QueueState_t  OsQueueRecieveISR(uint8_t QueueID,void * Message)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueueID >= 0 && QueueID < 16) {
		if (!DequeueEmpty(&QueueCBS[QueueID].Queue)) {
			void *Src = DequeueRemoveRear(&QueueCBS[QueueID].Queue);
			QueueCPYbuffer(Src, Message, QueueCBS[QueueID].ItemSize);
		} else {
			QueueState = QueueEmpty;
		}
	}
	OSExitCritical();
	return QueueState;
}
#elif OS_SCHEDULER_STATIC == FALSE
QueueState_t  OsQueueRecieve(pQueue QueuePtr,void * Message,uint8_t blocking)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if (QueuePtr != NULL) {
		if (blocking == OS_QUEUE_NBLOCK)
		{
			if(!DequeueEmpty(&QueuePtr->Queue))
			{
				void *Src = DequeueRemoveRear(&QueuePtr->Queue);
				QueueCPYbuffer(Src,Message,QueuePtr->ItemSize);
			}else{
				QueueState = QueueEmpty;
			}
		} else if (blocking == OS_QUEUE_BLOCK)
		{
			if (!DequeueEmpty(&QueuePtr->Queue)) {
				if (DequeueFull(&QueuePtr->Queue)) {
					TCB *CurrentTask = QueuePtr->OsSendingList.Front;
					// Queue Has An empty Location
					if(CurrentTask != NULL)
						QueueUnblockSending(QueuePtr);
				}
				void *Src = DequeueRemoveRear(&QueuePtr->Queue);
				QueueCPYbuffer(Src, Message, QueuePtr->ItemSize);
			} else {
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				QueuePtr->TasksWaitingSending++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList, NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				OsInsertQueueTail(&(QueuePtr->OsRecievingList), NewTask);\
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

QueueState_t  OsQueueRecieveISR(pQueue QueuePtr,void * Message)
{
	OSEnterCritical();
	QueueState_t QueueState = QueueOPSUCCESS;
	if(QueuePtr != NULL)
	{
		if (!DequeueEmpty(&QueuePtr->Queue))
		{
			void *Src = DequeueRemoveRear(&QueuePtr->Queue);
			QueueCPYbuffer(Src, Message, QueuePtr->ItemSize);
		} else {
			QueueState = QueueEmpty;
		}
	}
	OSExitCritical();
	return QueueState;
}

#endif
#if OS_SCHEDULER_STATIC == TRUE
QueueState_t  OsQueueRecieveBack(uint8_t QueueID,void * Message,uint8_t blocking)
{

}

QueueState_t  OsQueueRecieveFront(uint8_t QueueID,void * Message,uint8_t blocking)
{

}
#elif OS_SCHEDULER_STATIC == FALSE

#endif
#endif
#endif

static void QueueCPYbuffer(void * Src ,void *dest,int8_t Size)
{
	if(Src != NULL && dest != NULL)
	{

		uint8_t MemCounter_1 = 0;
		uint8_t MemCounter_2 = 0;
		uint8_t Size_1 = Size /SIZE_INT32;
		uint8_t Size_2 = Size %SIZE_INT32;
		if(Size_1 >0)
		{
			/*-------Copy Chunks of 4 bytes------*/
			for(MemCounter_1 = 0; MemCounter_1<= Size_1-1 ;MemCounter_1++ )
				*((uint32_t*)(dest) + MemCounter_1)=*(((uint32_t *)Src) + MemCounter_1 );
		}else{}
		if(Size_2 > 0)
		{
			MemCounter_1<<=INT32_SHIFT;
			for(MemCounter_2 = 0; MemCounter_2<= Size_2-1 ;MemCounter_2++ )
				*((uint8_t*)(dest) + MemCounter_1 +MemCounter_2)=*(((uint8_t *)Src) + MemCounter_1 +MemCounter_2 );
		}else{}
	}
}

#endif
