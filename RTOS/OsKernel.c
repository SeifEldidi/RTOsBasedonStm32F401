/*
 * OsKernel.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "OSKernel.h"

/*------Global Uinintialized Variables Stored in .bss Section in RAM---*/
/*-----Create a TCB for the Number of tasks Available in the System---*/
static TCB OS_TASKS[OS_TASKS_NUM+1];
int32_t    TCB_STACK[OS_TASKS_NUM+1][OS_STACK_SIZE];
int32_t    KernelStack[OS_KERNEL_STACK_SIZE];
int32_t    *kernelStackPtr = &KernelStack[OS_KERNEL_STACK_SIZE -1 ];
OsKernelControl KernelControl;
/*-----Create Global LinkedLists for Queues----*/
TCBLinkedList OsReadyList;
TCBLinkedList OsWaitingQueue;
TCBLinkedList OsSuspendedQueue;
TCB *         pCurrentTask;
TCB *         pCurrentTaskBlock;
/*----Global Control Variables----*/
uint32_t OS_TICK ;

static void  IdleTask();
static void  OsLuanchScheduler();
static void  osTaskDelayCheck();

void OsKernelTaskInit(uint32_t Thread_Index)
{
#if OS_FLOATING_POINT == DIS
	/*----------Init Stack Pointer to point to block below registers-----*/
	OS_TASKS[Thread_Index].Sptr = (int32_t *)(&TCB_STACK[Thread_Index][OS_STACK_SIZE-OS_R4_OFFSET]);
	/*--------Set T bit to 1------*/
	TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_XPSR_OFFSET] = OS_EPSR_TBIT_SET;
	/*-------Program Counter initialization---*/
	if(Thread_Index != OS_IDLE_TASK_OFFSET)
		TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_PC_OFFSET] = (uint32_t)(OS_TASKS[Thread_Index].TaskCode);
	else
	{
		OS_TASKS[Thread_Index].Sptr = (int32_t *)(&TCB_STACK[Thread_Index][OS_STACK_SIZE-OS_R4_OFFSET]);
		OS_TASKS[Thread_Index].TaskCode = (P2FUNC)(IdleTask);
		TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_PC_OFFSET] = (uint32_t)(IdleTask);
	}
	/*--------Init R0->R12-----*/
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_LR_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_R12_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R3_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R2_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R1_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R0_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_R11_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_R10_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R9_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R8_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R7_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R6_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R5_OFFSET] = 0xDEADDEAD;
	TCB_STACK[Thread_Index][OS_STACK_SIZE - OS_R4_OFFSET] = 0xDEADDEAD;
#endif
}

void  OsTaskSuspend(TaskHandle_t  TaskHandler)
{
	OSEnterCritical();
	TaskHandle_t Found = NULL;
	/*----Remove From Ready Queueu----*/
	Found = OsDequeQueueElement(&OsReadyList,TaskHandler);
	if(Found == NULL)
		Found = OsDequeQueueElement(&OsWaitingQueue,TaskHandler);
	if(Found != NULL)
	{
		/*----Should Check Suspended Queue Will be Added Later----*/
		TaskHandler->State = OS_TASK_SUSPENDED;
		TaskHandler->Next_Task = NULL ;
		TaskHandler->WaitingTime = -1;
		OsInsertQueueTail(&OsSuspendedQueue,TaskHandler);
	}
	OSExitCritical();
}

void  OsTaskResume(TaskHandle_t  TaskHandler)
{
	OSEnterCritical();
	TaskHandle_t Found = NULL;
	Found = OsDequeQueueElement(&OsSuspendedQueue,TaskHandler);
	if(Found != NULL)
	{
		OsInsertQueueTail(&OsReadyList,TaskHandler);
		TaskHandler->State = OS_TASK_READY;
		TaskHandler->Next_Task = NULL;
		TaskHandler->WaitingTime = -1;
	}
	OSExitCritical();
}

/*------RoundRobin Task----*/
uint8_t OSKernelAddThread(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,TaskHandle_t *Taskhandle)
{
	uint8_t OS_RET = OS_OK;
	OSEnterCritical();
	if(OsReadyList.No_Tasks == 0)
	{
		OS_TASKS[OsReadyList.No_Tasks].ID 		= ID;
		OS_TASKS[OsReadyList.No_Tasks].Priority  = Priority;
		OS_TASKS[OsReadyList.No_Tasks].TaskCode  = TaskCode;
		OS_TASKS[OsReadyList.No_Tasks].WaitingTime  = -1;

		if(Taskhandle != NULL)
			(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
		else{}
		OsInsertQueueHead(&OsReadyList,&OS_TASKS[OsReadyList.No_Tasks]);
		OsKernelTaskInit(OsReadyList.No_Tasks -1);
		OsKernelTaskInit(OS_IDLE_TASK_OFFSET);
	}else if(OsReadyList.No_Tasks < OS_TASKS_NUM && OsReadyList.No_Tasks > 0)
	{
		/*----Adding A Task while Executing----*/
		OS_TASKS[OsReadyList.No_Tasks].ID 		= ID;
		OS_TASKS[OsReadyList.No_Tasks].Priority  = Priority;
		OS_TASKS[OsReadyList.No_Tasks].TaskCode  = TaskCode;
		OS_TASKS[OsReadyList.No_Tasks].WaitingTime  = -1;

		if(Taskhandle != NULL)
			(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
		else{}
		OsInsertQueueTail(&OsReadyList,&OS_TASKS[OsReadyList.No_Tasks]);
		OsKernelTaskInit(OsReadyList.No_Tasks -1);
	}else{
		OS_RET = OS_NOT_OK;
	}
	OSExitCritical();
	return OS_RET;
}

void  OsKernelStart(uint32_t TimeQuanta)
{
	/*-----Configure Systick Timer-----*/
	SYSTICK_CFG SYS = {
			.LOAD_VAL = TimeQuanta,
			.CLCK_DIV = SYS_CLK_8,
			.SystickTick  = SYSTICK_MS,
	};
	HAL_SYSTICK_Init(&SYS);
	/*-----Set Current Task-----*/
	pCurrentTask  = OsReadyList.Front;
	/*-----Luanch Scheduler-----*/
	OsLuanchScheduler();
}

void OsDelay(uint32_t delayQuantaBased)
{
	OSEnterCritical();
	/*-----OSdelay Must Remove Task from Ready Queue and Insert it in the Blocking Queue----*/
	if(OsReadyList.No_Tasks >0)
	{
		if(delayQuantaBased > 0)
		{
			TCB *NewTask     = pCurrentTask ;
			NewTask->WaitingTime = delayQuantaBased;
			pCurrentTask->State = OS_TASK_BLOCKED;
			/*-----Remove Task From Ready Queue----*/
			OsDequeQueueElement(&OsReadyList,NewTask);
			/*-----Insert Task to Blocking Queue----*/
			OsInsertQueueTail(&OsWaitingQueue,NewTask);
			KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
			pCurrentTask = NewTask;
		}
	}else{

	}
	OSExitCritical();
	OsThreadYield(PRIVILEDGED_ACCESS);
}

void  OsThreadYield(uint8_t Privliedge)
{
	/*-----Force Reschedule----*/
	if(Privliedge == PRIVILEDGED_ACCESS)
		ICSR = PENDSV_PENDING;
	else{
		/*----Trigger SVC CALL---*/
	}
}

static void OsLuanchScheduler()
{
	/*----Disable Global Interrupts----*/
	OSEnterCritical();
	/*--------Load Address of the Current TCB Into R0-----*/
	__asm volatile ("LDR R0, =pCurrentTask");
	/*--------Helping Variables-----*/
	__asm volatile ("LDR R2 ,[R0]");
	__asm volatile ("LDR SP,[R2,#8]");
	/*-------Restore Registers-----*/
	__asm volatile ("POP {R4-R11}");
	__asm volatile ("POP {R0-R3}");
	__asm volatile ("POP {R12}");
	/*-----Skip LR-----*/
	__asm volatile ("ADD SP,SP,#4");
	/*----Create New Start Location----*/
	__asm volatile ("POP {LR}");
	__asm volatile ("ADD SP,SP,#4");
	/*----Restore Stack Pointer----*/
	OSExitCritical();
	__asm volatile ("BX LR");
}

void __attribute__((naked)) PendSV_Handler(void)
{
	 OSEnterCritical();
	 /*------Context Switch-----*/
	 /*1] Save R4-R11 to Stack*/
	 /*2] Save new Sp to Stack Pointer in TCB*/
	 __asm volatile ("PUSH {R4-R11}");
	 __asm volatile ("LDR R0, =pCurrentTask");
	 __asm volatile ("LDR R1,[R0]");
	 __asm volatile ("STR SP,[R1,#8]");
	 /*-----Switch To Kernel Stack----*/
	 __asm volatile ("LDR R1,=kernelStackPtr");
	 __asm volatile ("LDR SP,[R1]");
	 __asm volatile ("PUSH {R0,LR}");
	 //-----Recall stack frame of function is destroyed after function call i.e SP points to same location before func execution
	 __asm volatile ("BL  osTaskDelayCheck");
	 __asm volatile ("BL  osScheduler");
	 __asm volatile ("POP  {R0,LR}");
     /*------Context Switch of Previous Task-----*/
	/*1] Save R4-R11 to Stack*/
	/*2] Save new Sp to Stack Pointer in TCB*/
	/*--------Context Restore of Next Task------*/
	__asm volatile ("LDR R0, =pCurrentTask");
	__asm volatile ("LDR R1,[R0]");
	__asm volatile ("LDR SP,[R1,#8]");
	__asm volatile ("POP {R4-R11}");
	OSExitCritical();
	__asm volatile ("BX LR");
}

static void IdleTask()
{
	while(1)
	{

	}
}


void SysTick_Handler(void)
{
	KernelControl.OsTickPassed = 1;
	OS_TICK++;
	ICSR = PENDSV_PENDING;
}

void osScheduler()
{
	if(pCurrentTask->TaskCode == (P2FUNC)IdleTask)
	{
		if(OsReadyList.Front != NULL)
			pCurrentTask = OsReadyList.Front;
	}else{
		if(KernelControl.ContextSwitchControl == OS_CONTEXT_NORMAL)
		{
			OsReadyList.Front->State = OS_TASK_READY;
			TCB *Current = OsDequeQueueFront(&OsReadyList);
			OsInsertQueueTail(&OsReadyList, Current);
			/*-----Execute Task at the Front of the Queue---*/
			pCurrentTask = OsReadyList.Front;
			pCurrentTask->State = OS_TASK_RUNNING;
		}else if(KernelControl.ContextSwitchControl == OS_CONTEXT_BLOCKED)
		{
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			if(OsReadyList.Front != NULL)
				pCurrentTask = OsReadyList.Front;
			else
				pCurrentTask = &OS_TASKS[OS_IDLE_TASK_OFFSET];
			pCurrentTask->State = OS_TASK_RUNNING;
		}
	}
}

static void osTaskDelayCheck()
{
	TCB *FrontWaitingQueue = OsWaitingQueue.Front;
	while(FrontWaitingQueue != NULL)
	{
		if(KernelControl.OsTickPassed == 1)
		{
			FrontWaitingQueue->WaitingTime --;
			if(FrontWaitingQueue->WaitingTime == 0)
			{
				TCB *NewTask = FrontWaitingQueue;
				NewTask->WaitingTime = -1;
				/*------Wake the Task from Sleep------*/
				OsDequeQueueElement(&OsWaitingQueue,FrontWaitingQueue);
				/*------Add  Task to the Scheduler Queue----*/
				OsInsertQueueTail(&OsReadyList,NewTask);
			}
		}
		FrontWaitingQueue = FrontWaitingQueue->Next_Task;
	}
	KernelControl.OsTickPassed = 0;
}
