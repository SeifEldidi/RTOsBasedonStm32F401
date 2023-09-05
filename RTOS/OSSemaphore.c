/*
 * OSSemaphore.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "OSSemaphore.h"
#include "OSKernel.h"

extern TCB * pCurrentTask;
extern TCBLinkedList OsReadyList;
extern OsKernelControl KernelControl;
Semaphore OSSemaphoreCBS[OS_SEMAPHORE_NUMBER];

#if OS_SEMAPHORE_INIT == TRUE
SEMP_State_t OsSemaphoreInit (int8_t SemaphoreID,int8_t Value,char *Name)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		OSSemaphoreCBS[SemaphoreID].Value = Value;
		OSSemaphoreCBS[SemaphoreID].SemaphoreName = Name;
		OSSemaphoreCBS[SemaphoreID].TasksWaiting  = 0;
		OSSemaphoreCBS[SemaphoreID].OsSemaphoreList.Front = NULL;
		OSSemaphoreCBS[SemaphoreID].OsSemaphoreList.Rear  = NULL;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}
#endif

#if OS_SEMAPHORE_OBTAIN == TRUE
SEMP_State_t OsSemaphoreObtain(int8_t SemaphoreID,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if(SemaphoreID >=0 && SemaphoreID <OS_SEMAPHORE_NUMBER)
	{
		if(Blocking == OS_SEMAPHORE_NBLOCK)
		{
			/*----Available Semaphore-----*/
			if(OSSemaphoreCBS[SemaphoreID].Value > OS_SEMAPHORE_LOCK )
			{
				OSSemaphoreCBS[SemaphoreID].Value--;
			}else {
				Err_State = SEMPH_UNAVAILABLE;
			}
		}else{
			/*--------Wait For Semaphore-------*/
			if (OSSemaphoreCBS[SemaphoreID].Value > OS_SEMAPHORE_LOCK)
				OSSemaphoreCBS[SemaphoreID].Value--;
			else {
				 /*--------Wait For Semaphore by blocking Current Task Untill Semaphore is Released-------*/
				TCB *NewTask = pCurrentTask;
				pCurrentTask->State = OS_TASK_BLOCKED;
				OSSemaphoreCBS[SemaphoreID].TasksWaiting ++;
				/*---------Deque from Ready List --------*/
				OsDequeQueueElement(&OsReadyList,NewTask);
				/*---------Insert into Semaphore Waiting List----*/
				OsInsertQueueTail(&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),NewTask);
				/*---------When Task is blocked a reschedule is forced-----*/
				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsThreadYield(PRIVILEDGED_ACCESS);
			}
			Err_State = SEMPH_UNAVAILABLE;
		}
	}else{
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}
#endif


#if OS_SEMAPHORE_RELEASE == TRUE
SEMP_State_t OsSemaphoreRelease(int8_t SemaphoreID)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		if(OSSemaphoreCBS[SemaphoreID].Value <= 0)
		{
			TCB *CurrentTask = OSSemaphoreCBS[SemaphoreID].OsSemaphoreList.Front;
			/*------Release all Tasks blocked on This Semaphore----*/
			while(CurrentTask != NULL)
			{
				OSSemaphoreCBS[SemaphoreID].TasksWaiting--;
				OsDequeQueueElement(&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),CurrentTask);
				OsInsertQueueTail(&OsReadyList,CurrentTask);
				CurrentTask->State = OS_TASK_READY;
				CurrentTask = CurrentTask->Next_Task;
			}
			/*----No Reschedule is Forced----*/
			Err_State = SEMPH_SUCESS;
		}else{
		}
		OSSemaphoreCBS[SemaphoreID].Value++;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}
#endif

#if OS_SEMAPHORE_RELEASE == TRUE
SEMP_State_t OsSemaphoreReset(int8_t SemaphoreID)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		/*-------Set Semaphore As Available----*/
		OSSemaphoreCBS[SemaphoreID].Value = 1;
		/*------Release all Tasks blocked on This Semaphore----*/
		TCB *CurrentTask = OSSemaphoreCBS[SemaphoreID].OsSemaphoreList.Front;
		while (CurrentTask != NULL) {
			OsDequeQueueElement(&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),CurrentTask);
			OsInsertQueueTail(&OsReadyList, CurrentTask);
			CurrentTask->State = OS_TASK_READY;
			CurrentTask = CurrentTask->Next_Task;
		}
		/*----No Reschedule is Forced----*/
		Err_State = SEMPH_SUCESS;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}
#endif

#if OS_SEMAPHORE_COUNT == TRUE
SEMP_State_t OsSemaphoreCount(int8_t SemaphoreID,uint8_t *Val)
{
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		*Val = OSSemaphoreCBS[SemaphoreID].Value;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	return Err_State;
}
#endif

#if OS_SEMAPHORE_INFO  == TRUE
SEMP_State_t OsSemaphoreInfo(int8_t SemaphoreID,char * SemaphoreName,uint8_t * TasksWaiting,uint8_t *Value)
{
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		SemaphoreName = OSSemaphoreCBS[SemaphoreID].SemaphoreName;
		*Value = OSSemaphoreCBS[SemaphoreID].Value;
		*TasksWaiting = OSSemaphoreCBS[SemaphoreID].TasksWaiting;
		Err_State = SEMPH_SUCESS;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	return Err_State;
}
#endif
