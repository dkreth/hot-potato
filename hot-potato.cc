/*
 * Filename       hot-potato.cc
 * Date           2/24/2020
 * Author         Dylan Kreth
 * Email          dylan.kreth@utdallas.edu
 * Course         CS 4348.001 Spring 2020
 * Copyright      2020, All Rights Reserved
 * Procedures:
 * main - //TODO write main description
 * myOtherFunction - 
 */

#include <iostream>
#include <fstream>
#include "hot-potato.h"
#include <unistd.h>
#include <mqueue.h>

using namespace std;

/***************************************************************************
* int main( int argc, char **argv )
* Author: Dylan Kreth
* Date: 2/24/2020
* Description: //TODO: write main desc
*
* Parameters:
* argc I/P int The number of arguments on the command line
* argv I/P char *[] The arguments on the command line
* main O/P int Status code (not currently used)
**************************************************************************/
int main(int argc, char **argv)
{
	volatile int count = 0; //for counting the number of potatoes each child has received
	int fd[2];							//file descriptor for pipe
	//open a pipe for data transfer from child to parent
	if (pipe(fd) == -1) //pipe() returns -1 on error
	{
		cerr << "pipe() returned -1. " << endl; //errors should be reported
		perror("fork");													//send a pipe error //TODO
		exit(EXIT_FAILURE);											// if pipe fails, project is hosed, so just exit
	}

	//TODO create message queue for potato passing (dak160130)
	mqd_t mq = mq_open("/mq_dak160130", 0);

	//TODO fork 5 times so that we have 5 children. be careful about if() statements so that we don't get too many forks
	pid_t pid = 1; //start it as nonzero so that the parent can do the initial fork
	int numChildren = 0; //used later to track how many children there are left in the game
	for (numChildren = 0; (numChildren < NUM_CHILDREN) && (pid != 0); numChildren++)
	{
		if(DEBUG) cout << "forking " << numChildren << endl;
		pid = fork(); // we need more children bc we're Catholic
	}

	if (pid != 0) //parent process
	{
		while(true){
		mq_send(mq,"stuff",sizeof("stuff"),0); //TODO release the krakken!!! I mean potato
		if(DEBUG) cout << "in the parent" << getpid() <<endl;
		//TODO wait for a message on pipe (just read -- it's a blocking command). One will come in when someone hits 1000 catches
		//TODO store loser info (info came in on the pipe)
		//TODO send SIGUSR1 signal to all children. loser will terminate, rest will send their counts to the parent via the pipe (then reset count to zero)
		//TODO for loop thru remaining children, reading from pipe for each and recording their id and catch count
		//TODO if(numChildren == 1), break
		break;
		}
		//TODO share stats with user
		//TODO wait for last child(ren) to terminate
		//TODO unlink the message queue
	}

	else // one of the child processes
	{
		//TODO establish SIGUSR1 handler
		while(true){
			//TODO wait for potato (just read from message queue -- it's a blocking call (?) //TODO)
			//TODO increment counter bc we caught a potato
			if(count == 1000){ //potato explodes, this process just lost
				//TODO send message (id, count) on pipe, where parent will receive it
				//TODO wait for count to go to 0 (happens when signal handler is called)
				//TODO return after count goes to 0
			}
			else { //not a loss
				//TODO add a potato to the queue
			}
			if(DEBUG) cout << "in the child" << getpid() << endl;
			break;
		}
	}
	

	return 0;
}

/***************************************************************************
* int convertToMicroseconds(struct timeval time)
* Author: Dylan Kreth
* Date: 2/24/2020
* Description: //TODO write helper method description
* Parameters:
* time I/P struct timeval  the struct timeval of the time to be converted
* convertToMicroseconds O/P int number of microseconds
**************************************************************************/
int convertToMicroseconds(struct timeval time)
{
	return (time.tv_sec * 1000.0 * 1000) + (time.tv_usec);
}
