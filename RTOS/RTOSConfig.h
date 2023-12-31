/*
 * RTOSConfig.h
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */

#ifndef RTOSCONFIG_H_
#define RTOSCONFIG_H_

#define OS_FLOATING_POINT			DIS
#define OS_SCHEDULER_ROUND_ROBIN	0
#define OS_SCHEDULER_PRIORITY		1
#define OS_SCHEDULER_STATIC 		FALSE
#define OS_SCHEDULER_SELECT			OS_SCHEDULER_PRIORITY
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
#define OS_QUANTA 					1
#endif
#define OS_HEAP_SIZE				16000
#define OS_STACK_SIZE				200
#define OS_KERNEL_STACK_SIZE		100
/*-------Maximum No of Tasks-------*/
#define OS_TASKS_NUM				5
#define OS_MAX_PRIORITY				5

#define OS_EVENT_GROUP_16BITS		0

#define OS_SEMAPHORE_NUMBER 		16
#define OS_MUTEX_ENABLE				TRUE
#define OS_SEMAPHORE_OBTAIN 		TRUE
#define OS_SEMAPHORE_INIT			TRUE
#define OS_SEMAPHORE_RELEASE 		TRUE
#define OS_SEMAPHORE_RESET 			TRUE
#define OS_SEMAPHORE_COUNT 			TRUE
#define OS_SEMAPHORE_INFO			TRUE

#define OS_QUEUE_NUMBER				16
#define OS_QUEUE_SIZE				10
#define OS_QUEUE_SEND				TRUE
#define OS_QUEUE_RECIEVE			TRUE

#define OS_EVENT_GROUP_NUMBER

#endif /* RTOSCONFIG_H_ */
