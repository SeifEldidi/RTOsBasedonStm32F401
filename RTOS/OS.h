/*
 * OS.h
 *
 *  Created on: Sep 2, 2023
 *      Author: Seif pc
 */

#ifndef OS_H_
#define OS_H_

#include "../Inc/std_macros.h"
#include "core/CortexM4_core_NVIC.h"
#include "core/CortexM4_core_Systick.h"

#define OS_FLOATING_POINT		DIS
#define OS_STACK_SIZE			200
#define OS_KERNEL_STACK_SIZE	100
#define OS_TASKS_NUM			3 /*---Static SCHEDULING---*/
/*------Os Task States------*/
#define OS_TASK_READY			0
#define OS_TASK_BLOCKED			2
#define OS_TASK_RUNNING 		1
#define OS_TASK_SUSPENDED		3
/*------Os Kernel Definitions----*/
#define OS_EPSR_TBIT_SET		(1<<24)
#define OS_TASK_PRIORITY_MAX    7
#define OS_QUANTA 				2
#define OS_PERIOD				200

#define OS_SCHEDULER_SUSPENDED 	1
#define OS_SCHEDULER_RESUME		0
#define OS_CONTEXT_BLOCKED		1
#define OS_CONTEXT_NORMAL		0

#define OS_IDLE_TASK_OFFSET		OS_TASKS_NUM
#define OS_XPSR_OFFSET			1
#define OS_PC_OFFSET			2
#define OS_LR_OFFSET			3
#define OS_R12_OFFSET			4
#define OS_R3_OFFSET			5
#define OS_R2_OFFSET			6
#define OS_R1_OFFSET			7
#define OS_R0_OFFSET			8
#define OS_R11_OFFSET			9
#define OS_R10_OFFSET			10
#define OS_R9_OFFSET			11
#define OS_R8_OFFSET			12
#define OS_R7_OFFSET			13
#define OS_R6_OFFSET			14
#define OS_R5_OFFSET			15
#define OS_R4_OFFSET			16

#define OS_SEMAPHORE_AVAILABLE	1
#define OS_SEMAPHORE_LOCK		0
#define OS_SEMAPHORE_BLOCK		1
#define OS_SEMAPHORE_NBLOCK		0

#define ICSR					*((__IO uint32_t *)(0x04 + 0xE000ED00))
#define SYSTICK_PENDING			(1<<26)
#define PENDSV_PENDING			(1<<28)

#define MsToTicks(ms) (ms/OS_QUANTA)
#define OSEnterCritical() ({ __asm volatile ("CPSID i");})
#define OSExitCritical()  ({__asm volatile ("CPSIE i");})

typedef int32_t * StackPtr;
typedef void (*P2FUNC)(void);

typedef enum
{
	OS_OK,
	OS_NOT_OK,
	OS_NULL_PTR
}OS_ERRS;

typedef enum
{
	PendSvNormalContextSwitch,
	PendSvOSdelayContextSwitch,
}PendSVCALL;

typedef struct TCB
{
	uint8_t ID;
	uint8_t Priority;
	uint8_t State;
	P2FUNC  TaskCode;
	/*-----Stack pointer-------*/
	StackPtr Sptr;
	/*-----Pointer to Next TCB----*/
	struct TCB *Next_Task;
	/*-----Wating State---*/
	int32_t WaitingTime;
}TCB;

typedef TCB*    TaskHandle_t ;

typedef struct
{
	TCB * Front,*Rear;
	int32_t No_Tasks;
}TCBLinkedList;

typedef struct
{
	uint8_t ContextSwitchControl;
	uint8_t OsSchedulerSuspended;
	uint8_t OsTickPassed;
}OsKernelControl;


#endif /* OS_H_ */
