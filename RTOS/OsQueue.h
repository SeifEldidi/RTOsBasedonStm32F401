/*
 * OsQueue.h
 *
 *  Created on: Sep 6, 2023
 *      Author: Seif pc
 */

#ifndef OSQUEUE_H_
#define OSQUEUE_H_

#include "../Inc/std_macros.h"
#include "OS.h"
#include "OSKernel.h"
#include "MemManage.h"
#include "RTOSConfig.h"
#include "Deqeueu.h"

#define OS_QUEUE_BLOCK	1
#define OS_QUEUE_NBLOCK	0

#define SIZE_INT32		4
#define INT32_SHIFT		2

typedef struct
{
	char *QueueName;
	DeQueue Queue;
	uint8_t ItemSize;
	TCBLinkedList OsSendingList;
	TCBLinkedList OsRecievingList;
	uint8_t TasksWaitingSending;
	uint8_t TasksWaitingRecieving;
}QUEUCB_t;

typedef enum
{
	QueueFull,
	QueueEmpty,
	QueueInvalidID,
	QueueInvalidParamter,
	QueueOPSUCCESS,
}QueueState_t;

typedef QUEUCB_t   Queue;
typedef QUEUCB_t * pQueue;

void QueueCreateStatic(uint8_t QueueID,uint8_t QueuLen,char * QueueName,uint8_t ItemSize);
pQueue QueueCreateDynamic(uint8_t QueuLen,char * QueueName,uint8_t ItemSize);

#if OS_QUEUE_SEND == TRUE
#if OS_SCHEDULER_STATIC == TRUE
QueueState_t  OsQueueSend(uint8_t QueueID,void * Message,uint8_t blocking);
QueueState_t  OsQueueSendIsr(uint8_t QueueID,void * Message);
QueueState_t  OsQueueSendBack(uint8_t QueueID,void * Message,uint8_t blocking);
QueueState_t  OsQueueSendFront(uint8_t QueueID,void * Message,uint8_t blocking);
#elif OS_SCHEDULER_STATIC == FALSE
QueueState_t  OsQueueSend(pQueue QueuePtr,void * Message,uint8_t blocking);
QueueState_t  OsQueueSendIsr(pQueue QueuePtr,void * Message);
QueueState_t  OsQueueSendBack(pQueue QueuePtr,void * Message,uint8_t blocking);
QueueState_t  OsQueueSendFront(pQueue QueuePtr,void * Message,uint8_t blocking);
#endif
#endif

#if OS_QUEUE_RECIEVE == TRUE
#if OS_SCHEDULER_STATIC == TRUE
QueueState_t  OsQueueRecieve(uint8_t QueueID,void * Message,uint8_t blocking);
QueueState_t  OsQueueRecieveISR(uint8_t QueueID,void * Message)
QueueState_t  OsQueueRecieveBack(uint8_t QueueID,void * Message,uint8_t blocking);
QueueState_t  OsQueueRecieveFront(uint8_t QueueID,void * Message,uint8_t blocking);
#elif OS_SCHEDULER_STATIC == FALSE
QueueState_t  OsQueueRecieve(pQueue QueuePtr,void * Message,uint8_t blocking);
QueueState_t  OsQueueRecieveISR(pQueue QueuePtr,void * Message);
QueueState_t  OsQueueRecieveBack(pQueue QueuePtr,void * Message,uint8_t blocking);
QueueState_t  OsQueueRecieveFront(pQueue QueuePtr,void * Message,uint8_t blocking);
#endif
#endif

#endif /* OSQUEUE_H_ */
