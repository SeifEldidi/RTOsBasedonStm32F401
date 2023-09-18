/*
 * OSSemaphore.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "OSSemaphore.h"

extern TCB * pCurrentTask;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
extern TCBLinkedList OsReadyList;
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
extern TCBLinkedList OsReadyFIFO[OS_MAX_PRIORITY];
#endif
extern OsKernelControl KernelControl;

#if OS_SCHEDULER_STATIC == TRUE
static void OsBlockTask(TCBLinkedList * TargetList,TCBLinkedList *SrcList,uint8_t SemaphoreID)
{
	TCB *NewTask = pCurrentTask;
	pCurrentTask->State = OS_TASK_BLOCKED;
	OSSemaphoreCBS[SemaphoreID].TasksWaiting++;
	/*---------Deque from Ready List --------*/
	OsDequeQueueElement(SrcList, NewTask);
	/*---------Insert into Semaphore Waiting List----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
	OsInsertQueueTail(TargetList, NewTask);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	OsInsertQueueTail(&OsReadyFIFO[NewTask->Priority], NewTask);
#endif
	/*---------When Task is blocked a reschedule is forced-----*/
	KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
	OsThreadYield(PRIVILEDGED_ACCESS);
}
static void OsUnblockTasks(TCBLinkedList * TargetList,TCBLinkedList *SrcList,uint8_t SemaphoreID)
{
	TCB *CurrentTask = SrcList->Front;
	/*------Release First Task blocked on This Semaphore----*/
	if (CurrentTask != NULL) {
		OSSemaphoreCBS[SemaphoreID].TasksWaiting--;
		OsDequeQueueElement(SrcList, CurrentTask);
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueTail(&OsReadyFIFO[CurrentTask->Priority], CurrentTask);
#elif S_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(TargetList, CurrentTask);
#endif
		CurrentTask->State = OS_TASK_READY;
		CurrentTask = CurrentTask->Next_Task;
		if(CurrentTask->Priority > pCurrentTask->Priority)
			OsThreadYield(PRIVILEDGED_ACCESS);
	}else{}
}

static void OsUnblockAllTasks(TCBLinkedList * TargetList,TCBLinkedList *SrcList,uint8_t SemaphoreID)
{
	TCB *CurrentTask = SrcList->Front;
	/*------Release First Task blocked on This Semaphore----*/
	while(CurrentTask != NULL) {
		OSSemaphoreCBS[SemaphoreID].TasksWaiting--;
		OsDequeQueueElement(SrcList, CurrentTask);
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueTail(&OsReadyFIFO[CurrentTask->Priority], CurrentTask);
#elif S_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(TargetList, CurrentTask);
#endif
		CurrentTask->State = OS_TASK_READY;
		CurrentTask = CurrentTask->Next_Task;
	}
}

#elif OS_SCHEDULER_STATIC == FALSE
static void OsBlockTask(TCBLinkedList * TargetList,TCBLinkedList *SrcList,pSemaphore SemaphoreID)
{
	TCB *NewTask = pCurrentTask;
	pCurrentTask->State = OS_TASK_BLOCKED;
	SemaphoreID->TasksWaiting ++;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
	OsDequeQueueElement(SrcList, CurrentTask);
#endif
	/*---------Insert into Semaphore Waiting List----*/
	OsInsertQueueTail(TargetList, NewTask);
	/*---------When Task is blocked a reschedule is forced-----*/
	KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
	OsThreadYield(PRIVILEDGED_ACCESS);
}

static void OsUnblockTasks(TCBLinkedList * TargetList,TCBLinkedList *SrcList,pSemaphore SemaphoreID)
{
	TCB *CurrentTask = SrcList->Front;
	/*------Release Recent Task blocked on This Semaphore----*/
	if (CurrentTask != NULL) {
		SemaphoreID->TasksWaiting--;
		OsDequeQueueElement(SrcList, CurrentTask);
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueTail(&OsReadyFIFO[CurrentTask->Priority], CurrentTask);
#elif S_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(TargetList, CurrentTask);
#endif
		CurrentTask->State = OS_TASK_READY;
		CurrentTask = CurrentTask->Next_Task;
		if(CurrentTask->Priority > pCurrentTask->Priority)
			OsThreadYield(PRIVILEDGED_ACCESS);
	}else{}
}

static void OsUnblockAllTasks(TCBLinkedList * TargetList,TCBLinkedList *SrcList,pSemaphore SemaphoreID)
{
	TCB *CurrentTask = SrcList->Front;
	/*------Release all Tasks blocked on This Semaphore----*/
	while(CurrentTask != NULL) {
		SemaphoreID->TasksWaiting--;
		OsDequeQueueElement(SrcList, CurrentTask);
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueTail(&OsReadyFIFO[CurrentTask->Priority], CurrentTask);
#elif S_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(TargetList, CurrentTask);
#endif
		CurrentTask->State = OS_TASK_READY;
		CurrentTask = CurrentTask->Next_Task;
	}
}
#endif

#if OS_SCHEDULER_STATIC == FALSE

pSemaphore   OsSemaphoreCreateDyanmic(int8_t Value,char *Name,TaskHandle_t Task)
{
	OSEnterCritical();
	pSemaphore SemaphoreObject = NULL;
	SemaphoreObject = (pSemaphore)OsMalloc(sizeof(Semaphore)*1);
	if(SemaphoreObject != NULL)
	{
		SemaphoreObject->OsSemaphoreList.Front = NULL;
		SemaphoreObject->OsSemaphoreList.Rear  = NULL;
		SemaphoreObject->OsSemaphoreList.No_Tasks = 0;
		SemaphoreObject->OwnerTask = Task;
		SemaphoreObject->SemaphoreName = Name;
		SemaphoreObject->TasksWaiting = 0;
		SemaphoreObject->Value = Value;
		SemaphoreObject->OrigValue = Value;
	}else{
		SemaphoreObject = NULL;
	}
	OSExitCritical();
	return SemaphoreObject;
}

SEMP_State_t OsMutexObtainOwnership(pSemaphore Semaphore)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		if (Semaphore->OwnerTask == NULL)
			Semaphore->OwnerTask = pCurrentTask;
		else
			Err_State = SEMPH_INVALIDOwnership;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsMutexReleaseOwnership(pSemaphore Mutex)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Mutex != NULL) {
		if (pCurrentTask == Mutex->OwnerTask)
			Mutex->OwnerTask = NULL;
		else
			Err_State = SEMPH_INVALIDOwnership;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsSemaphoreObtain(pSemaphore Semaphore,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		if (Blocking == OS_SEMAPHORE_NBLOCK) {
			/*----Available Semaphore-----*/
			if (Semaphore->Value > OS_SEMAPHORE_LOCK) {
				Semaphore->Value--;
			} else {
				Err_State = SEMPH_UNAVAILABLE;
			}
		} else {
			/*--------Wait For Semaphore-------*/
			if (Semaphore->Value > OS_SEMAPHORE_LOCK)
				Semaphore->Value--;
			else {
				/*--------Wait For Semaphore by blocking Current Task Untill Semaphore is Released-------*/
				OsBlockTask(&(Semaphore->OsSemaphoreList),NULL,Semaphore);
			}
			Err_State = SEMPH_UNAVAILABLE;
		}
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsSemaphoreObtainISR(pSemaphore Semaphore,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
			/*----Available Semaphore-----*/
		if (Semaphore->Value > OS_SEMAPHORE_LOCK) {
			Semaphore->Value--;
		}else {

		}
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsSemaphoreRelease(pSemaphore Semaphore)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		if (Semaphore->Value <= 0) {
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
			OsUnblockTasks(NULL,
					&(Semaphore->OsSemaphoreList),
					Semaphore);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
			OsUnblockTasks(&OsReadyList,&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),
								SemaphoreID);
#endif
			/*----No Reschedule is Forced----*/
			Err_State = SEMPH_SUCESS;
		} else {
		}
		Semaphore->Value++;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsSemaphoreReleaseISR(pSemaphore Semaphore)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		if (Semaphore->Value <= 0) {
			TCB * ReleaseTask = Semaphore->OsSemaphoreList.Front;
			if(ReleaseTask != NULL)
			{
					//Remove Task from Waiting Queue
				   OsDequeQueueElement(&OsReadyFIFO[ReleaseTask->Priority], ReleaseTask);
			#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
				   OsInsertQueueTail(&OsReadyFIFO[ReleaseTask->Priority], ReleaseTask);
			#elif S_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
				   OsInsertQueueTail(TargetList, ReleaseTask);
			#endif
				if (ReleaseTask->Priority > pCurrentTask->Priority)
					OsThreadYield(PRIVILEDGED_ACCESS);
			}
			Err_State = SEMPH_SUCESS;
		} else {

		}
		Semaphore->Value++;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsSemaphoreReset(pSemaphore Semaphore)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		/*-------Set Semaphore As Available----*/
		Semaphore->Value = Semaphore->OrigValue;
		/*------Release all Tasks blocked on This Semaphore----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsUnblockAllTasks(NULL,
				&(Semaphore->OsSemaphoreList), Semaphore);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsUnblockAllTasks(&OsReadyList,
						&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList), Semaphore);
#endif
		/*----No Reschedule is Forced----*/
		Err_State = SEMPH_SUCESS;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}

SEMP_State_t OsSemaphoreCount(pSemaphore Semaphore,uint8_t *Val)
{
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		*Val = Semaphore->Value;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	return Err_State;
}

SEMP_State_t OsSemaphoreInfo(pSemaphore Semaphore,char * SemaphoreName,uint8_t * TasksWaiting,uint8_t *Value)
{
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Semaphore != NULL) {
		SemaphoreName = Semaphore->SemaphoreName;
		*Value = Semaphore->Value;
		*TasksWaiting = Semaphore->TasksWaiting;
		Err_State = SEMPH_SUCESS;
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	return Err_State;
}

SEMP_State_t OsMutexTake(pSemaphore Mutex,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Mutex != NULL) {
		if (Blocking == OS_SEMAPHORE_NBLOCK) {
			/*----Available Semaphore-----*/
			if (Mutex->Value == OS_SEMAPHORE_AVAILABLE)
				Mutex->Value = OS_SEMAPHORE_LOCK;
			else
				Err_State = SEMPH_UNAVAILABLE;
		} else {
			if (Mutex->Value == OS_SEMAPHORE_AVAILABLE)
				Mutex->Value = OS_SEMAPHORE_LOCK;
			else
			{
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
				OsBlockTask(&(Mutex->OsSemaphoreList),
						&OsReadyList, SemaphoreID);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
				OsBlockTask(&(Mutex->OsSemaphoreList), NULL,
						Mutex);
#endif
			}
		}
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}
SEMP_State_t OsMutexRelease(pSemaphore Mutex,uint8_t Blocking)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (Mutex != NULL) {
		if (Blocking == OS_SEMAPHORE_NBLOCK) {
			/*----Available Semaphore-----*/
			if (Mutex->OwnerTask == NULL) {
				Err_State = SEMPH_INVALIDOwnership;
			} else {
				if (Mutex->OwnerTask == pCurrentTask) {
					Mutex->Value = OS_SEMAPHORE_AVAILABLE;
				} else
					Err_State = SEMPH_UNAVAILABLE;
			}
		} else {
			if (Mutex->OwnerTask == NULL) {
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
				OsBlockTask(&(Mutex->OsSemaphoreList),&OsReadyList, SemaphoreID);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
				OsBlockTask(&(Mutex->OsSemaphoreList),NULL, Mutex);
#endif
				Err_State = SEMPH_UNAVAILABLE;
			} else {
				if (Mutex->Value == OS_SEMAPHORE_LOCK) {
					Mutex->Value = OS_SEMAPHORE_AVAILABLE;
					/*--------Unblock First Task into Task Waiting List----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
					OsUnblockTasks(&OsReadyList,
							&(Mutex->OsSemaphoreList),
							Mutex);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
					OsUnblockTasks(NULL, &(Mutex->OsSemaphoreList),
							Mutex);
#endif
				}
			}
		}
	} else {
		Err_State = SEMPH_INVALIDID;
	}
	OSExitCritical();
	return Err_State;
}


#elif OS_SCHEDULER_STATIC == TRUE
#if OS_SEMAPHORE_NUMBER >=0 && OS_SEMAPHORE_NUMBER<=16
Semaphore OSSemaphoreCBS[OS_SEMAPHORE_NUMBER];

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
			OsUnblockAllTasks(&OsReadyList,&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList),SemaphoreID);
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
SEMP_State_t OsSemaphoreReset(int8_t SemaphoreID)
{
	OSEnterCritical();
	SEMP_State_t Err_State = SEMPH_SUCESS;
	if (SemaphoreID >= 0 && SemaphoreID < OS_SEMAPHORE_NUMBER) {
		/*-------Set Semaphore As Available----*/
		OSSemaphoreCBS[SemaphoreID].Value = OSSemaphoreCBS[SemaphoreID].OrigValue;
		/*------Release all Tasks blocked on This Semaphore----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsUnblockAllTasks(NULL,
				&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList), SemaphoreID);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsUnblockAllTasks(&OsReadyList,
						&(OSSemaphoreCBS[SemaphoreID].OsSemaphoreList), SemaphoreID);
#endif
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
#endif
