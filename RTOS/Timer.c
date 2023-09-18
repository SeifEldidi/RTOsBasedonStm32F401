/*
 * Timer.c
 *
 *  Created on: Sep 18, 2023
 *      Author: Seif pc
 */
#include "Timer.h"

OsTimerDLL  DormantQueu;
OsTimerDLL  ReadyQueue;
extern int32_t OS_TICK;

#if OS_SCHEDULER_STATIC == FALSE
pOsTimer TimerCreate(uint8_t TimerOneShot_AutoReload,uint32_t TimerVal,TimerCallback Callback)
{
	OSEnterCritical();
	pOsTimer xTimer = (pOsTimer)OsMalloc(sizeof(OsTimer));
	if(xTimer != NULL)
	{
		xTimer->CallbackFunction = Callback;
		xTimer->TimerBaseVal = TimerVal;
		xTimer->TimerVal = xTimer->TimerBaseVal;
		xTimer->Timer_flag = TimerOneShot_AutoReload;
		xTimer->TimerState = OS_TIMER_STOP;
		//Timer is Stopped and is Added to Stop Queue
		OsTimerQueueInsertTail(&DormantQueu , xTimer);
	}else{
		xTimer = NULL;
	}
	OSExitCritical();
	return xTimer;
}

void     TimerStart(pOsTimer TimeHandle)
{
	OSEnterCritical();
	if(TimeHandle != NULL)
	{
		TimeHandle->TimerState = OS_TIMER_RUN;
		TimeHandle->CurrentTimeStamp = OS_TICK;
		/*-----Dequeu From Stop Queue-----*/
		OsTimerDequeQueueElement(&DormantQueu , TimeHandle);
		/*-----Add To Running Queue------*/
		OsTimerQueueInsertTail(&ReadyQueue , TimeHandle);
	}else{

	}
	OSExitCritical();
}

void     TimerStop(pOsTimer TimeHandle)
{
	OSEnterCritical();
	if (TimeHandle != NULL)
	{
		TimeHandle->TimerState = OS_TIMER_STOP;
		TimeHandle->TimerVal = TimeHandle->TimerBaseVal;
		/*-----Dequeu From Runing Queue-----*/
		OsTimerDequeQueueElement(&ReadyQueue, TimeHandle);
		/*-----Add To Dormant Queue------*/
		OsTimerQueueInsertTail(&DormantQueu, TimeHandle);
	}else{

	}
	OSExitCritical();
}

uint32_t TimerGetTick(pOsTimer TimeHandle)
{
	OSEnterCritical();
	uint32_t ReturnTime = 0;
	if (TimeHandle != NULL) {
		TimeHandle->TimerState = OS_TIMER_STOP;
		TimeHandle->TimerVal = TimeHandle->TimerBaseVal;
		ReturnTime = TimeHandle->TimerBaseVal - TimeHandle->TimerVal;
	}
	OSExitCritical();
	return ReturnTime;
}

void TimerDelayCheck()
{
	pOsTimer Head = ReadyQueue.Head;
	while(Head != NULL)
	{
		Head->TimerVal --;
		if(Head->TimerVal == 0)
		{
			if(Head->Timer_flag == OS_TIMER_ONESHOT)
			{
				Head->TimerState = OS_TIMER_STOP;
				/*-----Dequeu From Runing Queue-----*/
				OsTimerDequeQueueElement(&ReadyQueue, Head);
				/*-----Add To Dormant Queue------*/
				OsTimerQueueInsertTail(&DormantQueu, Head);
				/*-----Call Callback -----*/
				Head->CallbackFunction();
			}else{
				Head->TimerVal = Head->TimerBaseVal;
				Head->CallbackFunction();
			}
		}
		Head = Head->Next;
	}
}
#elif OS_SCHEDULER_STATIC == TRUE


#endif
