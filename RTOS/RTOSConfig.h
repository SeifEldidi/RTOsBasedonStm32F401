/*
 * RTOSConfig.h
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */

#ifndef RTOSCONFIG_H_
#define RTOSCONFIG_H_

#define OS_FLOATING_POINT			    DIS
#define OS_SCHEDULER_ROUND_ROBIN	0
#define OS_SCHEDULER_PRIORITY		  1
#define OS_SCHEDULER_SELECT			OS_SCHEDULER_PRIORITY
#if OS_SCHEDULER_SELECT == OS_SCHEDULER_PRIORITY
#define OS_QUANTA 					    1
#endif
#define OS_TASK_PRIORITY_MAX    	7

#define OS_HEAP_SIZE				    1024
#define OS_STACK_SIZE				    200
#define OS_KERNEL_STACK_SIZE		100
#define OS_TASKS_NUM				    5

#define OS_SEMAPHORE_NUMBER 		16
#define OS_SEMAPHORE_OBTAIN 		TRUE
#define OS_SEMAPHORE_INIT			  TRUE
#define OS_SEMAPHORE_RELEASE 		TRUE
#define OS_SEMAPHORE_RESET 			TRUE
#define OS_SEMAPHORE_COUNT 			TRUE
#define OS_SEMAPHORE_INFO			  TRUE

#define OS_QUEUE_NUMBER				16
#define OS_QUEUE_SIZE				  10
#define OS_QUEUE_SEND				  TRUE
#define OS_QUEUE_RECIEVE			TRUE


#endif /* RTOSCONFIG_H_ */
