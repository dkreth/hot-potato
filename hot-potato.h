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

#include <mqueue.h>
#include <iostream>
#include <fstream>

using namespace std;

#define DEBUG 0 //switch to 1 for DEBUG output to stdout
#define DEBUG2 0
#define DEBUG3 0

#define MAX_COUNT 1000
#define READ 0
#define WRITE 1
#define NUM_CHILDREN 5
#define RESULTS_FILE_NAME "results.txt"
#define MQ_NAME "/mq_dak160130b"

typedef struct Pack
{
    pid_t pid;
    int count;
} pack_t;

void signalHandler(int signal); //function prototype for helper
void waitOnPotato(mqd_t mq); //function prototype
void writeHeader(ofstream filestream, pack_t thisPack);
void openPipe(int* fd); //function prototype


#endif /* __FORKTIMER_H__ */
