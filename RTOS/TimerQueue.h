/*
 * TimerQueue.h
 *
 *  Created on: Sep 18, 2023
 *      Author: Seif pc
 */

#ifndef TIMERQUEUE_H_
#define TIMERQUEUE_H_

#include "OS.h"

typedef void (*TimerCallback)(void);

typedef struct OsTimer_t
{
	uint8_t  TimerState;
	int32_t  TimerVal;
	int32_t  CurrentTimeStamp;
	uint32_t TimerBaseVal;
	uint8_t  Timer_flag;
	TimerCallback CallbackFunction;
	struct OsTimer_t * Next;
	struct OsTimer_t * Prev;
}OsTimer_t;

typedef struct
{
	OsTimer_t * Head;
	OsTimer_t * Tail;
	int16_t     No_Timers;
}OsTimerDLL;

void OsTimerQueueInsertHead(OsTimerDLL * List, OsTimer_t *Elem);
void OsTimerQueueInsertTail(OsTimerDLL * List, OsTimer_t *Elem);
OsTimer_t *OsTimerDequeQueueElement(OsTimerDLL * List,OsTimer_t *Elem);
OsTimer_t *OsTimerDequeQueueRear(OsTimerDLL * List);
OsTimer_t *OsTimerDequeQueueFront(OsTimerDLL * List);

#endif /* TIMERQUEUE_H_ */
