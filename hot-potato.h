/*
 * Filename       fork-timer.h
 * Date           1/27/2020
 * Author         Dylan Kreth
 * Email          dylan.kreth@utdallas.edu
 * Course         CE 438.001 Spring 2020
 * Copyright      2020, All Rights Reserved
 */

#ifndef __FORKTIMER_H__
#define __FORKTIMER_H__

#define DEBUG 0 //switch to 1 for DEBUG output to stdout
#define READ 0
#define WRITE 1
#define NUM_CHILDREN 5

int convertToMicroseconds(struct timeval time); //TODO add function prototype for helper

#endif /* __FORKTIMER_H__ */
