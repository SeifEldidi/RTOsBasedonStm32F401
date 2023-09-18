/*
 * Timer.h
 *
 *  Created on: Sep 18, 2023
 *      Author: Seif pc
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "RTOSConfig.h"
#include "MemManage.h"
#include "TimerQueue.h"

#define OS_TIMER_ONESHOT		0
#define OS_TIMER_AUTOReload  	1

#define OS_TIMER_STOP		0
#define OS_TIMER_RUN		1

typedef OsTimer_t 	OsTimer;
typedef OsTimer_t * pOsTimer;

#if OS_SCHEDULER_STATIC == FALSE
pOsTimer TimerCreate(uint8_t TimerOneShot_AutoReload,uint32_t TimerVal,TimerCallback Callback);
void     TimerStart(pOsTimer TimeHandle);
void     TimerStop(pOsTimer TimeHandle);
uint32_t TimerGetTick(pOsTimer TimeHandle);
void 	 TimerDelayCheck();
#elif OS_SCHEDULER_STATIC == TRUE

#endif

#endif /* TIMER_H_ */
