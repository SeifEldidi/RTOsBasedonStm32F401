/*
 * OsKernel.c
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */
#include "OSKernel.h"

/*------Global Uinintialized Variables Stored in .bss Section in RAM---*/
/*-----Create a TCB for the Number of tasks Available in the System---*/
#if OS_SCHEDULER_STATIC == TRUE
static TCB OS_TASKS[OS_TASKS_NUM+1];
int32_t    TCB_STACK[OS_TASKS_NUM+1][OS_STACK_SIZE];
#endif
int32_t     KernelStack[OS_KERNEL_STACK_SIZE];
int32_t    *kernelStackPtr = &KernelStack[OS_KERNEL_STACK_SIZE -1 ];
OsKernelControl KernelControl;
/*-----Create Global LinkedLists for Queues----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
TCBLinkedList OsReadyFIFO[OS_MAX_PRIORITY];
static void osPriorityFindNextTask();
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
TCBLinkedList OsReadyList;
#endif
TCBLinkedList OsWaitingQueue;
TCBLinkedList OsSuspendedQueue;
TCB *         pCurrentTask;
TCB *         pCurrentTaskBlock;
TCB *         IdleTaskPtr;
int32_t       IdleTaskTime;
/*----Global Control Variables----*/
uint32_t OS_TICK ;

static void  IdleTask();
static void  OsLuanchScheduler();
static void  OsUserMode();
void osTaskDelayCheck();


#if OS_SCHEDULER_STATIC == TRUE
static void 	OsKernelTaskInitStatic(uint32_t Thread_Index);
#elif OS_SCHEDULER_STATIC == FALSE
static void     OsKernelTaskInitDynamic(uint32_t StackSize,StackPtr Stack,TCB *Task,uint8_t Flag);
#endif

#if OS_SCHEDULER_STATIC == TRUE
static uint8_t OsTaskCreateStatic(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,TaskHandle_t *Taskhandle);
#endif

#if OS_SCHEDULER_STATIC == FALSE
static uint8_t OsTaskCreateDynamic(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,uint32_t StackSize,TaskHandle_t *Taskhandle);
#endif

static void  OsUserMode()
{
	/*-----Set Stack Pointer to PSP---*/
	__asm volatile("MRS R0 , CONTROL");
	__asm volatile("ORR R0,R0,#2");
	__asm volatile("MSR CONTROL,R0");
}

#if OS_SCHEDULER_STATIC == FALSE
static uint8_t OsTaskCreateDynamic(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,uint32_t StackSize,TaskHandle_t *Taskhandle)
{
	uint8_t OS_RET = OS_OK;
	OSEnterCritical();
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
	//Allocate Memory for TASK Stack and Task Control Block
	if(OsReadyList.No_Tasks < OS_TASKS_NUM )
	{
		TCB *TaskN = (TCB*) OsMalloc(1 * sizeof(TCB));
		TCB *IdleTCB = (TCB*) OsMalloc(1 * sizeof(TCB));
		if (OsReadyList.No_Tasks == 0) {
			StackPtr TaskStack = (StackPtr) OsMalloc(StackSize * sizeof(OS_STACK_ALLIGN));
			StackPtr IDLETask = (StackPtr) OsMalloc(50 * sizeof(OS_STACK_ALLIGN));
		}
		if (TaskN != NULL && TaskStack != NULL) {
			TaskN->ID = ID;
			TaskN->Priority = Priority;
			TaskN->TaskCode = TaskCode;
			TaskN->WaitingTime = -1;
			TaskN->CurrQueue = OsReadyList;

			if (Taskhandle != NULL)
				(*Taskhandle) = TaskN;
			else {
			}
			OsInsertQueueHead(&OsReadyList, TaskN);
			OsKernelTaskInitDynamic(StackSize, TaskStack, TaskN, OS_NORMAL_TASK);
			if(OsReadyList.No_Tasks == 0)
			{
				if (IDLETask != NULL && IdleTCB != NULL)
				{
					IdleTCB->Priority = 0;
					OsKernelTaskInitDynamic(50, IDLETask, IdleTCB, OS_IDLE_TASK);
				}
			}else{
				OS_RET = OS_MEM_ERR;
			}
		} else {
			OS_RET = OS_MEM_ERR;
		}
	}else{
		OS_RET = OS_MEM_ERR;
	}
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	static uint8_t TaskCounter = 0;
	//insert Only into Head if Highest priority
	if (TaskCounter == 0) {
		//Allocate Memory for TASK Stack and Task Control Block
		TCB *TaskN = (TCB*) OsMalloc(1 * sizeof(TCB));
		TCB *IdleTCB = (TCB*)OsMalloc(1*sizeof(TCB));
		StackPtr TaskStack = (StackPtr) OsMalloc(StackSize * sizeof(OS_STACK_ALLIGN));
		StackPtr IDLETask = (StackPtr) OsMalloc(50 * sizeof(OS_STACK_ALLIGN));
		if(TaskN != NULL && TaskStack != NULL)
		{
			if (Priority > OS_MAX_PRIORITY)
				Priority = OS_MAX_PRIORITY;
			else {
			}
			TaskN->ID = ID;
			TaskN->Priority = Priority;
			TaskN->TaskCode = TaskCode;
			TaskN->WaitingTime = -1;
			TaskN->CurrQueue = &OsReadyFIFO[TaskN->Priority];


			if (Taskhandle != NULL)
				(*Taskhandle) = TaskN;

			OsInsertQueueHead(&OsReadyFIFO[TaskN->Priority], TaskN);

			OsKernelTaskInitDynamic(StackSize,TaskStack,TaskN,OS_NORMAL_TASK);;
			if(IDLETask != NULL && IdleTCB!= NULL)
			{
				IdleTCB->Priority = 0;
				OsKernelTaskInitDynamic(50,IDLETask,IdleTCB,OS_IDLE_TASK);
			}
			TaskCounter++;
		}
		else {
			OS_RET = OS_MEM_ERR;
		}
	} else if (TaskCounter > 0 && TaskCounter < OS_TASKS_NUM) {
		/*----Adding A Task Dynamically----*/
		TCB *TaskN = (TCB*) OsMalloc(1 * sizeof(TCB));
		StackPtr TaskStack = (StackPtr) OsMalloc(StackSize * sizeof(OS_STACK_ALLIGN));
		if(TaskN != NULL && TaskStack != NULL)
		{
			if (Priority > OS_MAX_PRIORITY)
				Priority = OS_MAX_PRIORITY;
			else {
			}
			TaskN->ID = ID;
			TaskN->Priority = Priority;
			TaskN->TaskCode = TaskCode;
			TaskN->WaitingTime = -1;
			TaskN->CurrQueue = &OsReadyFIFO[TaskN->Priority];

			OsInsertQueueTail(&OsReadyFIFO[TaskN->Priority], TaskN);

			if (Taskhandle != NULL)
				(*Taskhandle) = TaskN;
			else {
			}
			OsKernelTaskInitDynamic(StackSize,TaskStack,TaskN,OS_NORMAL_TASK);
			TaskCounter++;
		}else{
			OS_RET = OS_MEM_ERR;
		}
	} else {
		OS_RET = OS_NOT_OK;
	}
#endif
	OSExitCritical();
	return OS_RET;
}
#endif

#if OS_SCHEDULER_STATIC == TRUE
static uint8_t OsTaskCreateStatic(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,TaskHandle_t *Taskhandle)
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
			OS_TASKS[OsReadyList.No_Tasks].CurrQueue  = OsReadyList;

			if(Taskhandle != NULL)
				(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
			else{}
			OsInsertQueueHead(&OsReadyList,&OS_TASKS[OsReadyList.No_Tasks]);
			OsKernelTaskInitStatic(OsReadyList.No_Tasks -1);
			OsKernelTaskInitStatic(OS_IDLE_TASK_OFFSET);
		}else if(OsReadyList.No_Tasks < OS_TASKS_NUM && OsReadyList.No_Tasks > 0)
		{
			/*----Adding A Task while Executing----*/
			OS_TASKS[OsReadyList.No_Tasks].ID 		= ID;
			OS_TASKS[OsReadyList.No_Tasks].Priority  = Priority;
			OS_TASKS[OsReadyList.No_Tasks].TaskCode  = TaskCode;
			OS_TASKS[OsReadyList.No_Tasks].WaitingTime  = -1;
			OS_TASKS[OsReadyList.No_Tasks].CurrQueue  = OsReadyList;

			if(Taskhandle != NULL)
				(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
			else{}
			OsInsertQueueTail(&OsReadyList,&OS_TASKS[OsReadyList.No_Tasks]);
			OsKernelTaskInitStatic(OsReadyList.No_Tasks -1);
		}else{
			OS_RET = OS_NOT_OK;
		}
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		//insert Only into Head if Highest priority
		static uint8_t TaskCounter = 0;
		if(TaskCounter == 0)
		{
			if (Priority > OS_MAX_PRIORITY)
				Priority = OS_MAX_PRIORITY;
			else {
			}
			OS_TASKS[OsReadyList.No_Tasks].ID = ID;
			OS_TASKS[OsReadyList.No_Tasks].Priority = Priority;
			OS_TASKS[OsReadyList.No_Tasks].TaskCode = TaskCode;
			OS_TASKS[OsReadyList.No_Tasks].WaitingTime = -1;
			OS_TASKS[OsReadyList.No_Tasks].CurrQueue  = &OsReadyFIFO[OS_TASKS[OsReadyList.No_Tasks].Priority];

			if (Taskhandle != NULL)
				(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
			else {}

			OsInsertQueueHead(&OsReadyFIFO[OS_TASKS[OsReadyList.No_Tasks].Priority], &OS_TASKS[OsReadyList.No_Tasks]);
			OsKernelTaskInitStatic(OsReadyList.No_Tasks - 1);
			OsKernelTaskInitStatic(OS_IDLE_TASK_OFFSET);
			TaskCounter++;
		}else if(TaskCounter < OS_TASKS_NUM && TaskCounter > 0)
		{
			TCB * Prev = NULL;
			TCB * pCurrentTaskL = OsReadyList.Front;
			uint8_t TaskPriority = Priority;
			/*----Adding A Task while Executing----*/
			if (Priority > OS_MAX_PRIORITY)
				Priority = OS_MAX_PRIORITY;
			else {
			}
			OS_TASKS[OsReadyList.No_Tasks].ID = ID;
			OS_TASKS[OsReadyList.No_Tasks].Priority = Priority;
			OS_TASKS[OsReadyList.No_Tasks].TaskCode = TaskCode;
			OS_TASKS[OsReadyList.No_Tasks].WaitingTime = -1;
			OS_TASKS[OsReadyList.No_Tasks].CurrQueue  = &OsReadyFIFO[OS_TASKS[OsReadyList.No_Tasks].Priority];

			OsInsertQueueTail(&OsReadyFIFO[OS_TASKS[OsReadyList.No_Tasks].Priority], &OS_TASKS[OsReadyList.No_Tasks]);

			if (Taskhandle != NULL)
				(*Taskhandle) = &OS_TASKS[OsReadyList.No_Tasks];
			else {}
			OsKernelTaskInitStatic(OsReadyList.No_Tasks - 1);
			TaskCounter++;
		}else{
				OS_RET = OS_NOT_OK;
		}
	#endif
		OSExitCritical();
		return OS_RET;
}
#endif

#if OS_SCHEDULER_STATIC == TRUE
static void OsKernelTaskInitStatic(uint32_t Thread_Index)
{
#if OS_FLOATING_POINT == DIS
	/*----------Init Stack Pointer to point to block below registers-----*/
	OS_TASKS[Thread_Index].Sptr = (int32_t *)(&TCB_STACK[Thread_Index][OS_STACK_SIZE-OS_R4_OFFSET]);
	OS_TASKS[Thread_Index].TopStack = OS_TASKS[Thread_Index].Sptr;
	OS_TASKS[Thread_Index].EndStack = OS_TASKS[Thread_Index].Sptr - OS_STACK_SIZE;
	/*--------Set T bit to 1------*/
	TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_XPSR_OFFSET] = OS_EPSR_TBIT_SET;
	/*-------Program Counter initialization---*/
	if(Thread_Index != OS_IDLE_TASK_OFFSET)
		TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_PC_OFFSET] = (uint32_t)(OS_TASKS[Thread_Index].TaskCode);
	else
	{
		OS_TASKS[Thread_Index].Priority = 0;
		OS_TASKS[Thread_Index].Sptr = (int32_t *)(&TCB_STACK[Thread_Index][OS_STACK_SIZE-OS_R4_OFFSET]);
		OS_TASKS[Thread_Index].TaskCode = (P2FUNC)(IdleTask);
		TCB_STACK[Thread_Index][OS_STACK_SIZE -OS_PC_OFFSET] = (uint32_t)(IdleTask);
		IdleTaskPtr =(TCB*) (&OS_TASKS[Thread_Index]);
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
#elif OS_SCHEDULER_STATIC == FALSE
static void OsKernelTaskInitDynamic(uint32_t StackSize,StackPtr Stack,TCB *Task,uint8_t Flag)
{
#if OS_FLOATING_POINT == DIS
	/*----------Init Stack Pointer to point to block below registers-----*/
	Task->Sptr =(int32_t*) (&Stack[OS_STACK_SIZE - OS_R4_OFFSET]);
	Task->TopStack = Stack + StackSize;
	Task->EndStack = Stack;
	/*--------Set T bit to 1------*/
	Stack[OS_STACK_SIZE - OS_XPSR_OFFSET] = OS_EPSR_TBIT_SET;
	/*-------Program Counter initialization---*/
	if (Flag != OS_IDLE_TASK)
		Stack[OS_STACK_SIZE - OS_PC_OFFSET] =(uint32_t) (Task->TaskCode);
	else {
		Task->Sptr =(int32_t*) (&Stack[OS_STACK_SIZE- OS_R4_OFFSET]);
		Task->TaskCode = (P2FUNC) (IdleTask);
		Stack[OS_STACK_SIZE - OS_PC_OFFSET] =(uint32_t) (IdleTask);
		IdleTaskPtr = Task;
	}
	/*--------Init R0->R12-----*/
	Stack[OS_STACK_SIZE - OS_LR_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R12_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R3_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R2_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R1_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R0_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R11_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R10_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R9_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R8_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R7_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R6_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R5_OFFSET] = 0xDEADDEAD;
	Stack[OS_STACK_SIZE - OS_R4_OFFSET] = 0xDEADDEAD;
#endif
}
#endif

void  OsTaskSuspend(TaskHandle_t  TaskHandler)
{
	OSEnterCritical();
	TaskHandle_t Found = NULL;
	/*----Remove From Ready Queue----*/
	Found = OsDequeQueueElement(TaskHandler->CurrQueue,TaskHandler);
	if(Found != NULL)
	{
		/*----Should Check Suspended Queue Will be Added Later----*/
		TaskHandler->State = OS_TASK_SUSPENDED;
		TaskHandler->Next_Task = NULL ;
		TaskHandler->WaitingTime = -1;
		TaskHandler->CurrQueue = &OsSuspendedQueue;

#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueSorted(&OsSuspendedQueue,TaskHandler);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(&OsSuspendedQueue,TaskHandler);
#endif
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
		TaskHandler->CurrQueue = &OsReadyFIFO[TaskHandler->Priority];
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueTail(&OsReadyFIFO[TaskHandler->Priority], TaskHandler);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(&OsReadyList,TaskHandler);
#endif
		TaskHandler->State = OS_TASK_READY;
		TaskHandler->Next_Task = NULL;
		TaskHandler->WaitingTime = -1;
	}
	OSExitCritical();
}

/*------RoundRobin Task----*/

uint8_t OSKernelAddThread(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,uint32_t StackSize,TaskHandle_t *Taskhandle)
{
	uint8_t OS_RET = OS_OK;
	OSEnterCritical();
#if OS_SCHEDULER_STATIC == TRUE
	OsTaskCreateStatic(TaskCode,ID,Priority,Taskhandle);
#elif OS_SCHEDULER_STATIC == FALSE
	OsTaskCreateDynamic(TaskCode,ID,Priority,StackSize,Taskhandle);
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
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	osPriorityFindNextTask();
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
	pCurrentTask  = OsReadyList.Front;
#endif
	/*-----Switch To Stack of the User----*/
	OsUserMode();
	/*-----Luanch Scheduler-----*/
	OsLuanchScheduler();
}

void OsDelay(uint32_t delayQuantaBased)
{
	OSEnterCritical();
	/*-----OSdelay Must Remove Task from Ready Queue and Insert it in the Blocking Queue----*/
	if (delayQuantaBased > 0) {
		TCB *NewTask = pCurrentTask;
		NewTask->WaitingTime = delayQuantaBased;
		pCurrentTask->State = OS_TASK_BLOCKED;
		/*-----Remove Task From Ready Queue----*/
		/*-----Insert Task to Blocking Queue----*/
		pCurrentTask->CurrQueue = &OsWaitingQueue;
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
		OsInsertQueueSorted(&OsWaitingQueue, NewTask);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
		OsInsertQueueTail(&OsWaitingQueue,NewTask);
#endif
		KernelControl.ContextSwitchControl = OS_CONTEXT_BLOCKED;
		pCurrentTask = NewTask;
	}
	else{

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
	 if(KernelControl.ContextSwitchControl != OS_CONTEXT_KILL)
	 {
		 /*------Context Switch-----*/
		 /*3] Save R4-R11 to Stack*/
		 /*4] Save new Sp to Stack Pointer in TCB*/
		 /*------Before Pushung to Stack Set MSP to PSP Location----*/
		 __asm volatile ("MRS R0,PSP");
		 __asm volatile ("MOV SP,R0");
		 /*-----Push To Task Stack-------*/
		 __asm volatile ("PUSH {R4-R11}");
		 __asm volatile ("LDR R0, =pCurrentTask");
		 __asm volatile ("LDR R1,[R0]");
		 __asm volatile ("STR SP,[R1,#8]");
	 }
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
     /*------Context Switch of Previous Task-----*/
	/*1] Save R4-R11 to Stack*/
	/*2] Save new Sp to Stack Pointer in TCB*/
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

void __attribute__((naked)) SysTick_Handler(void)
{
	KernelControl.OsTickPassed = 1;
	OS_TICK++;
	ICSR = PENDSV_PENDING;
	__asm volatile("BX LR");
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
		}else if(KernelControl.ContextSwitchControl == OS_CONTEXT_KILL)
		{
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			#if OS_SCHEDULER_STATIC == FALSE
				TCB *NextTaskP = OsReadyList.Front;
				if (NextTaskP != NULL) {
					pCurrentTask = NextTaskP;
				} else
				{
					pCurrentTask = IdleTaskPtr;
					pCurrentTask->State = OS_TASK_RUNNING;
				}
			#endif
		}
	}
}
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY

static void osPriorityFindNextTask()
{
	uint8_t PriorityFIFO = 0;
	// Look For Next Task and Select Highest Task in ReadyFIFO
	for(PriorityFIFO = 1 ; PriorityFIFO <= OS_MAX_PRIORITY -1 ;PriorityFIFO++)
	{
		TCB * HeadQueue = OsReadyFIFO[PriorityFIFO].Front;
		if(HeadQueue != NULL)
		{
			/*--------Remove Task from FIFO-------*/
			OsDequeQueueFront(&OsReadyFIFO[PriorityFIFO]);
			/*--------Insert Task into ReadyList-----*/
			pCurrentTask = HeadQueue;
			return;
		}
	}
	pCurrentTask = IdleTaskPtr;
}

//What Happens is that Tasks with Same Priority operate on a Round-Robin basis
void osPriorityScheduler()
{
	//Idle Task CODE
	if (pCurrentTask->TaskCode == (P2FUNC) IdleTask) {
		osPriorityFindNextTask();
		IdleTaskTime++;
	} else {
		if (KernelControl.ContextSwitchControl == OS_CONTEXT_NORMAL) {
			//Insert Current Executing Task To Tail of Priority List
			OsInsertQueueTail(&OsReadyFIFO[pCurrentTask->Priority],pCurrentTask);
			//Look For Next Task
			osPriorityFindNextTask();
			pCurrentTask->State = OS_TASK_RUNNING;
		} else if (KernelControl.ContextSwitchControl == OS_CONTEXT_BLOCKED) {
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			osPriorityFindNextTask();
			pCurrentTask->State = OS_TASK_RUNNING;
		}else if(KernelControl.ContextSwitchControl == OS_CONTEXT_KILL)
		{
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			#if OS_SCHEDULER_STATIC == FALSE
				osPriorityFindNextTask();
				pCurrentTask ->State = OS_TASK_RUNNING;
			#endif
		}
		pCurrentTask->CurrQueue = NULL;
	}
}
#endif

void osTaskDelayCheck()
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
				#if  OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY

				#endif
				OsDequeQueueElement(&OsWaitingQueue, FrontWaitingQueue);
				/*------Add  Task to the Scheduler Queue----*/
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_ROUND_ROBIN
				OsInsertQueueTail(&OsReadyList, NewTask);
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
				KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
				OsInsertQueueTail(&OsReadyFIFO[NewTask->Priority], NewTask);
#endif
			}
			FrontWaitingQueue = FrontWaitingQueue->Next_Task;
		}
	}
	KernelControl.OsTickPassed = 0;
}


#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
void OsPrioritySet(TaskHandle_t Taskhandle,uint8_t Priority)
{
	if(Taskhandle != NULL)
	{
		OsDequeQueueElement((TCBLinkedList *)&Taskhandle->CurrQueue, Taskhandle);
		Taskhandle->Priority = Priority ;
		OsInsertQueueSorted((TCBLinkedList *)&Taskhandle->CurrQueue, Taskhandle);
		if ((pCurrentTask && pCurrentTask->Priority < Priority)|| pCurrentTask == IdleTaskPtr)
			OsThreadYield(PRIVILEDGED_ACCESS);
	}else{
		pCurrentTask->Priority = Priority;
	}
}
#endif

#if OS_SCHEDULER_STATIC == FALSE
void    OsKillTask(TaskHandle_t Taskhandle)
{
	OSEnterCritical();
	TCB *CurrTask = NULL;
	uint8_t ContextSwitch = 0;
	if(Taskhandle == NULL)
	{
		/*-------Kill Current Task------*/
		CurrTask = pCurrentTask;
		/*-------Reqeust Context Switch----*/
		KernelControl.ContextSwitchControl = OS_CONTEXT_KILL;
		ContextSwitch = 1;
	}else{
		//Remove the Task from the Queue it is Placed Inside
		OsDequeQueueElement(Taskhandle->CurrQueue,Taskhandle);
		CurrTask = Taskhandle;
	}
	/*------Free Stack-----*/
	OsFree(CurrTask->EndStack);
	/*------Free TCB------*/
	OsFree(CurrTask);
	/*------Switch Context to Next Task----*/
	if(ContextSwitch == 1)
		OsThreadYield(PRIVILEDGED_ACCESS);
	else{}
	OSExitCritical();
}
#endif
