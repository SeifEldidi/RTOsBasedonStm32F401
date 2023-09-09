/*
 * OSSemaphore.h
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */

#ifndef OSSEMAPHORE_H_
#define OSSEMAPHORE_H_
#include "../Inc/std_macros.h"
#include "OS.h"
#include "OSKernel.h"
#include "QueueControl.h"
#include "RTOSConfig.h"

#define OS_SEMAPHORE_AVAILABLE	1
#define OS_SEMAPHORE_LOCK		0

#define OS_SEMAPHORE_BLOCK		1
#define OS_SEMAPHORE_NBLOCK		0

#if OS_SEMAPHORE_NUMBER >0 && OS_SEMAPHORE_NUMBER<=16
typedef enum
{
	SEMPH_SUCESS,
	SEMPH_UNAVAILABLE,
	SEMPH_INVALIDID,
	SEMPH_INVALIDPTR,
	SEMPH_INVALIDOwnership
}SEMP_State_t;

typedef struct
{
	char * SemaphoreName;
	int8_t Value;
#if OS_MUTEX_ENABLE == TRUE
	TCB *OwnerTask;
#endif
	TCBLinkedList OsSemaphoreList;
	uint8_t TasksWaiting;
}SemaphoreCB_t ;

typedef SemaphoreCB_t   Semaphore;
typedef SemaphoreCB_t * pSemaphore;

#if OS_SEMAPHORE_INIT == TRUE
SEMP_State_t OsMutexObtainOwnership(int8_t SemaphoreID);
SEMP_State_t OsMutexReleaseOwnership(int8_t SemaphoreID);
SEMP_State_t OsSemaphoreInit (int8_t SemaphoreID,int8_t Value,char *Name,TaskHandle_t Task);
#endif

#if OS_SEMAPHORE_OBTAIN == TRUE
SEMP_State_t OsSemaphoreObtain(int8_t SemaphoreID,uint8_t Blocking);
SEMP_State_t OsMutexTake(int8_t SemaphoreID,uint8_t Blocking);
#endif

#if OS_SEMAPHORE_RELEASE == TRUE
SEMP_State_t OsSemaphoreRelease(int8_t SemaphoreID);
SEMP_State_t OsMutexRelease(int8_t SemaphoreID,uint8_t Blocking);
#endif

#if OS_SEMAPHORE_RESET == TRUE
SEMP_State_t OsSemaphoreReset(int8_t SemaphoreID);
SEMP_State_t OsMutexReset(int8_t SemaphoreID,int8_t Value,char *Name);
#endif

#if OS_SEMAPHORE_COUNT == TRUE
SEMP_State_t OsSemaphoreCount(int8_t SemaphoreID,uint8_t *Val);
#endif

#if OS_SEMAPHORE_INFO  == TRUE
SEMP_State_t OsSemaphoreInfo(int8_t SemaphoreID,char * SemaphoreName,uint8_t * TasksWaiting,uint8_t *Value);
#endif
#endif

#endif /* OSSEMAPHORE_H_ */
