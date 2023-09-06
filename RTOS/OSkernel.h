/*
 * OSkernel.h
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */

#ifndef OSKERNEL_H_
#define OSKERNEL_H_

#include "OS.h"
#include "QueueControl.h"

#define PRIVILEDGED_ACCESS  0
#define NPRIVILEDGED_ACCESS 1

void 	OsKernelTaskInit(uint32_t Thread_Index);
uint8_t OSKernelAddThread(P2FUNC TaskCode,uint8_t ID,uint8_t Priority,TaskHandle_t *Taskhandle);
void 	OsKernelStart();
void 	OsDelay(uint32_t delayQuantaBased);
void    OsTaskSuspend(TaskHandle_t  TaskHandler);
void    OsTaskResume(TaskHandle_t  TaskHandler);
void    OsThreadYield(uint8_t Privliedge);
void    PeriodicTask();


#endif /* OSKERNEL_H_ */
