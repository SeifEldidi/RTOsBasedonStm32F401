/*
 * QueueControl.h
 *
 *  Created on: Sep 5, 2023
 *      Author: Seif pc
 */

#ifndef QUEUECONTROL_H_
#define QUEUECONTROL_H_

#include "OS.h"

void  OsInsertQueueHead(TCBLinkedList * Queue,TCB *Elem);
void  OsInsertQueueTail(TCBLinkedList * Queue,TCB *Elem);
TCB * OsDequeQueueElement(TCBLinkedList * Queue,TCB *Elem);
TCB * OsDequeQueueRear(TCBLinkedList * Queue);
TCB * OsDequeQueueFront(TCBLinkedList * Queue);

#endif /* QUEUECONTROL_H_ */
