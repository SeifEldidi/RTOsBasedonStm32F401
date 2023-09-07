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
static void  OsUserMode();

static void  OsUserMode()
{
	/*-----Set Stack Pointer to PSP---*/
	__asm volatile("MRS R0 , CONTROL");
	__asm volatile("ORR R0,R0,#2");
	__asm volatile("MSR CONTROL,R0");
}

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
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
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
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	//insert Only into Head if Highest priority
	if(OsReadyList.No_Tasks == 0)
	{
		OS_TASKS[OsReadyList.No_Tasks].ID = ID;
		OS_TASKS[OsReadyList.No_Tasks].Priority = Priority;
		OS_TASKS[OsReadyList.No_Tasks].TaskCode = TaskCode;
		OS_TASKS[OsReadyList.No_Tasks].WaitingTime = -1;

		if (Taskhandle != NULL)
			(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
		else {}
		OsInsertQueueHead(&OsReadyList, &OS_TASKS[OsReadyList.No_Tasks]);
		OsKernelTaskInit(OsReadyList.No_Tasks - 1);
		OsKernelTaskInit(OS_IDLE_TASK_OFFSET);
	}else if(OsReadyList.No_Tasks < OS_TASKS_NUM && OsReadyList.No_Tasks > 0)
	{
		TCB * Prev = NULL;
		TCB * pCurrentTaskL = OsReadyList.Front;
		uint8_t TaskPriority = Priority;
		/*----Adding A Task while Executing----*/
		OS_TASKS[OsReadyList.No_Tasks].ID = ID;
		OS_TASKS[OsReadyList.No_Tasks].Priority = Priority;
		OS_TASKS[OsReadyList.No_Tasks].TaskCode = TaskCode;
		OS_TASKS[OsReadyList.No_Tasks].WaitingTime = -1;

		while(pCurrentTaskL != NULL)
		{
			//List is Sorted according to Priority
			if(TaskPriority > pCurrentTaskL->Priority)
			{
				if(Prev != NULL)
				{
					OS_TASKS[OsReadyList.No_Tasks].Next_Task = pCurrentTaskL;
					Prev->Next_Task = &OS_TASKS[OsReadyList.No_Tasks];
				}else
					OsInsertQueueHead(&OsReadyList, &OS_TASKS[OsReadyList.No_Tasks]);
				break;
			}else{
				Prev = pCurrentTaskL;
				pCurrentTaskL = pCurrentTaskL->Next_Task;
			}
		}
		if(pCurrentTaskL == NULL)
			OsInsertQueueTail(&OsReadyList, &OS_TASKS[OsReadyList.No_Tasks]);

		if (Taskhandle != NULL)
			(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
		else {}
		OsKernelTaskInit(OsReadyList.No_Tasks - 1);
	}else{
			OS_RET = OS_NOT_OK;
	}
#endif
	OSExitCritical();
	return OS_RET;
}

void  OsKernelStart()
{
	/*-----Configure Systick Timer-----*/
	SYSTICK_CFG SYS ;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
	SYS.LOAD_VAL = OS_QUANTA;
#endif
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	SYS.LOAD_VAL = 1;
#endif
	SYS.CLCK_DIV = SYS_CLK_8;
	SYS.SystickTick  = SYSTICK_MS;
	HAL_SYSTICK_Init(&SYS);
	/*----Set MSP to Point to Kernel Stack----*/
	__asm volatile("LDR R0,=kernelStackPtr");
	__asm volatile("LDR R1,[R0]");
	__asm volatile("MSR MSP ,R1");
	/*-----Set Current Task-----*/
	pCurrentTask  = OsReadyList.Front;
	/*-----Switch To Stack of the User----*/
	OsUserMode();
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
	 /*------Before Pushing to Stack Set MSP to PSP Location----*/
	 __asm volatile ("MRS R0,PSP");
	 __asm volatile ("MOV SP,R0");
	 /*-----Push To Task Stack-------*/
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
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
	 __asm volatile ("BL  osRoundRobinScheduler");
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	 __asm volatile ("BL  osPriorityScheduler");
#endif
	 __asm volatile ("POP  {R0,LR}");
        /*------Context Switch of Previous Task Complete-----*/
	/*1] pop R4-R11 of Next Task*/
	/*2] Save new Sp to Stack Pointer in TCB*/
	/*3] Copy the Value of the MSP into PSP */
	/*--------Context Restore of Next Task------*/
	__asm volatile ("LDR R0, =pCurrentTask");
	__asm volatile ("LDR R1,[R0]");
	__asm volatile ("LDR SP,[R1,#8]");
	__asm volatile ("POP {R4-R11}");
	__asm volatile ("MRS R0 , MSP");
	__asm volatile ("MSR PSP, R0");
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

#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
void osRoundRobinScheduler()
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
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
//What Happens is that Tasks with Same Priority operate on a Round-Robin basis
void osPriorityScheduler()
{
	//Idle Task CODE
	if (pCurrentTask->TaskCode == (P2FUNC) IdleTask) {
		if (OsReadyList.Front != NULL)
			pCurrentTask = OsReadyList.Front;
	} else {
		if (KernelControl.ContextSwitchControl == OS_CONTEXT_NORMAL) {
			//Remove Currently Executing Task and place at the Tail of the Tasks with same Priority -> Round Robin Scheduling
			OsDequeQueueFront(&OsReadyList);
			TCB * NextTaskP = OsReadyList.Front;
			TCB * PrevTask = NULL;
			uint8_t TaskPriority   = pCurrentTask->Priority;
			if(NextTaskP != NULL)
			{
				while(NextTaskP->Priority >= TaskPriority)
				{
					PrevTask = NextTaskP;
					NextTaskP = NextTaskP->Next_Task;
				}
				if(PrevTask != NULL)
				{
					pCurrentTask->Next_Task = PrevTask->Next_Task;
					PrevTask->Next_Task = pCurrentTask;
				}else{
					OsReadyList.Front = pCurrentTask;
					pCurrentTask->Next_Task = NextTaskP;
				}
				OsReadyList.No_Tasks++;
			}else
				OsInsertQueueHead(&OsReadyList,pCurrentTask);
			pCurrentTask = OsReadyList.Front;
			pCurrentTask->State = OS_TASK_RUNNING;
		} else if (KernelControl.ContextSwitchControl == OS_CONTEXT_BLOCKED) {
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			if (OsReadyList.Front != NULL)
				pCurrentTask = OsReadyList.Front;
			else
				pCurrentTask = &OS_TASKS[OS_IDLE_TASK_OFFSET];
			pCurrentTask->State = OS_TASK_RUNNING;
		}
	}
}
#endif

static void osTaskDelayCheck()
{
	TCB *FrontWaitingQueue = OsWaitingQueue.Front;
	if(KernelControl.OsTickPassed == 1)
	{
		while(FrontWaitingQueue != NULL)
		{
			FrontWaitingQueue->WaitingTime--;
			if (FrontWaitingQueue->WaitingTime == 0) {
				TCB *NewTask = FrontWaitingQueue;
				NewTask->WaitingTime = -1;
				/*------Wake the Task from Sleep------*/
				OsDequeQueueElement(&OsWaitingQueue, FrontWaitingQueue);
				/*------Add  Task to the Scheduler Queue----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
				OsInsertQueueTail(&OsReadyList, NewTask);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
				KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
				OsInsertQueueSorted(&OsReadyList, NewTask);
#endif
			}
			FrontWaitingQueue = FrontWaitingQueue->Next_Task;
		}
	}
	KernelControl.OsTickPassed = 0;
}
