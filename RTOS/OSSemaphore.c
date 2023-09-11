/*
 * OSSemaphore.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "OSSemaphore.h"

extern TCB * pCurrentTask;
extern TCBLinkedList OsReadyList;
extern OsKernelControl KernelControl;


#if OS_SEMAPHORE_NUMBER >=0 && OS_SEMAPHORE_NUMBER<=16
Semaphore OSSemaphoreCBS[OS_SEMAPHORE_NUMBER];

static void OsBlockTask(TCBLinkedList * TargetList,TCBLinkedList *SrcList,uint8_t SemaphoreID)
{
	TCB *NewTask = pCurrentTask;
	pCurrentTask->State = OS_TASK_BLOCKED;
	OSSemaphoreCBS[SemaphoreID].TasksWaiting++;
	/*---------Deque from Ready List --------*/
	OsDequeQueueElement(SrcList, NewTask);
	/*---------Insert into Semaphore Waiting List----*/
	OsInsertQueueTail(TargetList, NewTask);
	/*---------When Task is blocked a reschedule is forced-----*/
	KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
	OsThreadYield(PRIVILEDGED_ACCESS);
}

static void OsUnblockTasks(TCBLinkedList * TargetList,TCBLinkedList *SrcList,uint8_t SemaphoreID)
{
	TCB *CurrentTask = SrcList->Front;
	/*------Release all Tasks blocked on This Semaphore----*/
	while (CurrentTask != NULL) {
		OSSemaphoreCBS[SemaphoreID].TasksWaiting--;
		OsDequeQueueElement(SrcList,	CurrentTask);
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueSorted(TargetList, CurrentTask);
#elif S_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(TargetList, CurrentTask);
#endif
		CurrentTask->State = OS_TASK_READY;
		CurrentTask = CurrentTask->Next_Task;
	}
}

#if OS_SEMAPHORE_INIT == TRUE
SEMP_State_t OsSemaphoreInit (int8_t SemaphoreID,int8_t Value,char *Name,TaskHandle_t Task)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		OSSemaphoreCBS[SemaphoreID].Value = Value;
		OSSemaphoreCBS[SemaphoreID].SemaphoreName = Name;
		OSSemaphoreCBS[SemaphoreID].TasksWaiting  = 0;
		OSSemaphoreCBS[SemaphoreID].OsSemaphoreList.Front = NULL;
		OSSemaphoreCBS[SemaphoreID].OsSemaphoreList.Rear  = NULL;
		OSSemaphoreCBS[SemaphoreID].OwnerTask  = Task;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsMutexObtainOwnership(int8_t SemaphoreID)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		if (OSSemaphoreCBS[SemaphoreID].OwnerTask == NULL)
			OSSemaphoreCBS[SemaphoreID].OwnerTask = pCurrentTask;
		else
			Err_State = SEMPH_INVALIDOwnership;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsMutexReleaseOwnership(int8_t SemaphoreID)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		if(pCurrentTask == OSSemaphoreCBS[SemaphoreID].OwnerTask)
			OSSemaphoreCBS[SemaphoreID].OwnerTask = NULL;
		else
			Err_State = SEMPH_INVALIDOwnership;
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
				OsBlockTask(&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),&OsReadyList,SemaphoreID);
			}
			Err_State = SEMPH_UNAVAILABLE;
		}
	}else{
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

/*-----Any Task Can Take the Mutex but only the Owner Task Can Release It---*/
SEMP_State_t OsMutexTake(int8_t SemaphoreID,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		if (Blocking == OS_SEMAPHORE_NBLOCK) {
			/*----Available Semaphore-----*/
			if(OSSemaphoreCBS[SemaphoreID].Value == OS_SEMAPHORE_AVAILABLE)
				OSSemaphoreCBS[SemaphoreID].Value = OS_SEMAPHORE_LOCK;
			else
				Err_State = SEMPH_UNAVAILABLE;
		} else {
			if (OSSemaphoreCBS[SemaphoreID].Value == OS_SEMAPHORE_AVAILABLE)
				OSSemaphoreCBS[SemaphoreID].Value = OS_SEMAPHORE_LOCK;
			else
				OsBlockTask(&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),&OsReadyList,SemaphoreID);
		}
	} else {
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
			OsUnblockTasks(&OsReadyList,&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),SemaphoreID);
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

SEMP_State_t OsMutexRelease(int8_t SemaphoreID,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		if (Blocking == OS_SEMAPHORE_NBLOCK) {
			/*----Available Semaphore-----*/
			if (OSSemaphoreCBS[SemaphoreID].OwnerTask == NULL) {
				Err_State = SEMPH_INVALIDOwnership;
			} else {
				if(OSSemaphoreCBS[SemaphoreID].OwnerTask == pCurrentTask)
				{
					OSSemaphoreCBS[SemaphoreID].Value = OS_SEMAPHORE_AVAILABLE;
				}
				else
					Err_State = SEMPH_UNAVAILABLE;
			}
		} else {
			if (OSSemaphoreCBS[SemaphoreID].OwnerTask == NULL) {
				OsBlockTask(&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),&OsReadyList,SemaphoreID);
				Err_State = SEMPH_UNAVAILABLE;
			} else {
				if (OSSemaphoreCBS[SemaphoreID].Value == OS_SEMAPHORE_LOCK)
				{
					OSSemaphoreCBS[SemaphoreID].Value = OS_SEMAPHORE_AVAILABLE;
					/*--------Unblock First Task into Task Waiting List----*/
					OsUnblockTasks(&OsReadyList,&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),SemaphoreID);
				}
			}
		}
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
		OsUnblockTasks(&OsReadyList,&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),SemaphoreID);
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

#endif
