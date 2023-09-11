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
TCBLinkedList OsReadyList;
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

		if (Taskhandle != NULL)
			(*Taskhandle) = TaskN;
		else {
		}
		OsInsertQueueHead(&OsReadyList, TaskN);
		OsKernelTaskInitDynamic(StackSize, TaskStack, TaskN, OS_NORMAL_TASK);
		if(OsReadyList.No_Tasks == 0)
		{
			if (IDLETask != NULL && IdleTCB != NULL)
				OsKernelTaskInitDynamic(50, IDLETask, IdleTCB, OS_IDLE_TASK);
		}else{
			OS_RET = OS_MEM_ERR;
		}
	} else {
		OS_RET = OS_MEM_ERR;
	}
#elif OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
	//insert Only into Head if Highest priority
	if (OsReadyList.No_Tasks == 0) {
		//Allocate Memory for TASK Stack and Task Control Block
		TCB *TaskN = (TCB*) OsMalloc(1 * sizeof(TCB));
		TCB *IdleTCB = (TCB*)OsMalloc(1*sizeof(TCB));
		StackPtr TaskStack = (StackPtr) OsMalloc(StackSize * sizeof(OS_STACK_ALLIGN));
		StackPtr IDLETask = (StackPtr) OsMalloc(50 * sizeof(OS_STACK_ALLIGN));
		if(TaskN != NULL && TaskStack != NULL)
		{
			TaskN->ID = ID;
			TaskN->Priority = Priority;
			TaskN->TaskCode = TaskCode;
			TaskN->WaitingTime = -1;

			if (Taskhandle != NULL)
				(*Taskhandle) = TaskN;
			OsInsertQueueHead(&OsReadyList, TaskN);
			OsKernelTaskInitDynamic(StackSize,TaskStack,TaskN,OS_NORMAL_TASK);;
			if(IDLETask != NULL && IdleTCB!= NULL)
				OsKernelTaskInitDynamic(50,IDLETask,IdleTCB,OS_IDLE_TASK);
		}
		else {
			OS_RET = OS_MEM_ERR;
		}
	} else if (OsReadyList.No_Tasks > 0) {
		TCB *Prev = NULL;
		TCB *pCurrentTaskL = OsReadyList.Front;
		uint8_t TaskPriority = Priority;
		/*----Adding A Task while Executing----*/
		TCB *TaskN = (TCB*) OsMalloc(1 * sizeof(TCB));
		StackPtr TaskStack = (StackPtr) OsMalloc(StackSize * sizeof(OS_STACK_ALLIGN));
		if(TaskN != NULL && TaskStack != NULL)
		{
			TaskN->ID = ID;
			TaskN->Priority = Priority;
			TaskN->TaskCode = TaskCode;
			TaskN->WaitingTime = -1;

			while (pCurrentTaskL != NULL) {
				//List is Sorted according to Priority
				if (TaskPriority > pCurrentTaskL->Priority) {
					if (Prev != NULL) {
						TaskN->Next_Task = pCurrentTaskL;
						Prev->Next_Task = TaskN;
					} else
						OsInsertQueueHead(&OsReadyList,TaskN);
					break;
				} else {
					Prev = pCurrentTaskL;
					pCurrentTaskL = pCurrentTaskL->Next_Task;
				}
			}
			if (pCurrentTaskL == NULL)
				OsInsertQueueTail(&OsReadyList, TaskN);

			if (Taskhandle != NULL)
				(*Taskhandle) = TaskN;
			else {
			}
			OsKernelTaskInitDynamic(StackSize,TaskStack,TaskN,OS_NORMAL_TASK);
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
			OsKernelTaskInitStatic(OsReadyList.No_Tasks - 1);
			OsKernelTaskInitStatic(OS_IDLE_TASK_OFFSET);
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
			OsKernelTaskInitStatic(OsReadyList.No_Tasks - 1);
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
		IdleTaskTime++;
	} else {
		if (KernelControl.ContextSwitchControl == OS_CONTEXT_NORMAL) {
			//Remove Currently Executing Task and place at the Tail of the Tasks with same Priority Round Robin Priority
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
				if(NextTaskP == NULL)
					OsReadyList.Rear = pCurrentTask;
				else{}
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
				pCurrentTask = IdleTaskPtr;
			pCurrentTask->State = OS_TASK_RUNNING;
		}else if(KernelControl.ContextSwitchControl == OS_CONTEXT_KILL)
		{
			KernelControl.ContextSwitchControl = OS_CONTEXT_NORMAL;
			#if OS_SCHEDULER_STATIC == FALSE
				OsDequeQueueFront(&OsReadyList);
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

void osTaskStackOverflow()
{
	if(pCurrentTask->Sptr < pCurrentTask->EndStack )
	{

	}
}


#if OS_SCHEDULER_STATIC == FALSE
void    OsKillTask(TaskHandle_t Taskhandle)
{
	OSEnterCritical();
	TCB *CurrTask = NULL;
	if(Taskhandle == NULL)
	{
		/*-------Kill Current Task------*/
		CurrTask = pCurrentTask;
	}else{
		CurrTask = Taskhandle;
	}
	KernelControl.ContextSwitchControl = OS_CONTEXT_KILL;
	/*------Free Stack-----*/
	OsFree(CurrTask->EndStack);
	/*------Free TCB------*/
	OsFree(CurrTask);
	/*------Switch Context to Next Task----*/
	OsThreadYield(PRIVILEDGED_ACCESS);
	OSExitCritical();
}
#endif
