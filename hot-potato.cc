/*
 * Filename       hot-potato.cc
 * Date           2/24/2020
 * Author         Dylan Kreth
 * Email          dylan.kreth@utdallas.edu
 * Course         CS 4348.001 Spring 2020
 * Copyright      2020, All Rights Reserved
 * Procedures:
 * main - plays hot potato between
 * myOtherFunction - 
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include "hot-potato.h"

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

//Global variables bc they're accessed by signalHandler
volatile int count = 0; //for counting the number of potatoes each child has received
pack_t thisPack = {getpid(),0}; // holder for data to be sent on the pipe
int fd[2];//file descriptor for pipe


int main(int argc, char **argv)
{
	
	//open a pipe for data transfer from child to parent
	if (pipe(fd) == -1) //pipe() returns -1 on error
	{
		cerr << "pipe() returned -1. " << endl; //errors should be reported
		perror("pipe");													//send a pipe error //TODO
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
		if((pid = fork()) < 0){ // fork returns negative int on error
			perror("fork");
			exit(1);
		}
	}	

	if (pid != 0) //parent process
	{
		ofstream filestream; //open a file to write results to
		filestream.open(RESULTS_FILE_NAME); //using const is best practice. defined in hot-potato.h
		close(fd[WRITE]);
		signal(SIGUSR1, SIG_IGN);
		while(true){
			mq_send(mq,"stuff",sizeof("stuff"),0); // release the krakken!!! I mean potato
				if(DEBUG2) cout << "PARENT: sent potato" << endl;
				if(DEBUG) cout << "in the parent" << getpid() <<endl;
				if(DEBUG2) cout << "PARENT: waiting for message on pipe" << endl;
			read(fd[READ], &thisPack, sizeof(thisPack)); //TODO wait for a message on pipe (just read -- it's a blocking command). One will come in when someone hits 1000 catches
				if(DEBUG2) cout << "PARENT: got a message on pipe. writing to file" << endl;
			// writeHeader(filestream, thisPack);
			filestream << "LOSER DETECTED: " <<thisPack.pid << " " << thisPack.count << endl; //TODO store loser info (info came in on the pipe)
			filestream << "\nROUND FINISHED\nRESULTS FROM THIS ROUND:\n\tPID\tCOUNT" << endl;

			// filestream << "LOSER DETECTED: " <<thisPack.pid << " " << thisPack.count << endl; //TODO store loser info (info came in on the pipe)
			// filestream << "\nROUND FINISHED\nRESULTS FROM THIS ROUND:\n\tPID\tCOUNT" << endl;
			if(DEBUG3) cout << "PARENT: sending signal" << endl;
			kill(0,SIGUSR1);//TODO send SIGUSR1 signal to all children. loser will terminate, rest will send their counts to the parent via the pipe (then reset count to zero)
			if(DEBUG3) cout << "PARENT: signal sent" << endl;
			//TODO for loop thru children, reading from pipe for each and recording their id and catch count
			for(int i = 0; i < numChildren; i++){
				read(fd[READ], &thisPack, sizeof(thisPack));
				filestream << "\t" << thisPack.pid << "\t" << thisPack.count << endl;
			}
			numChildren--;
			//TODO if(numChildren == 1), break
			if(numChildren == 1)
				break;
		}

		//TODO share stats with user
		//TODO wait for last child(ren) to terminate
		//TODO unlink the message queue
		if(mq_close(mq) < 0)
			cerr << "error closing the message queue" << endl;
		filestream.close(); //we're done writing
	}

	else // one of the child processes
	{
		close(fd[READ]);
		signal(SIGUSR1, signalHandler); //establish SIGUSR1 handler
		if(DEBUG) cout << "CHILD" << getpid() << ": set up signalHandler" << endl;
		while(true){
			waitOnPotato(mq);
			count++; // increment counter bc we caught a potato
			if(DEBUG2) cout << "CHILD" << getpid() << ": got a potato. count is now " << count << endl;
			if(count >= MAX_COUNT){ //potato explodes, this process just lost
				if(DEBUG2) cout << "CHILD" << getpid() << ": my count hit 1000" << endl;
				thisPack.count = count; //update thisPack with data so it'll be correct when we send it to the parent
				thisPack.pid = getpid(); //update thisPack
				write(fd[WRITE], &thisPack, sizeof(thisPack)); //send message (id, count) on pipe, where parent will receive it
				if(DEBUG3) cout << "CHILD" << getpid() << ": I lost! waiting for count to be set to 0." << endl;
				while(count !=0){ //wait for count to go to 0 (happens when signal handler is called)
					//busy waiting loop, do nothing
				}
				if(DEBUG3) cout << "CHILD" << getpid() << ": my count was set to 0. time to die."  << endl;
				return 0; //return bc we're done
			}
			else { //not a loss
				if(DEBUG2) cout << "CHILD" << getpid() << ": putting potato back" << endl;
				mq_send(mq,"stuff",sizeof("stuff"),0); //add a potato to the queue
				if(DEBUG2) cout << "CHILD" << getpid() << ": put potato back" << endl;
			}
			if(DEBUG) cout << "in the child" << getpid() << endl;
			if(DEBUG3) cout << "CHILD" << getpid() << " count: " << count << endl;
		}
	}
	

	return 0;
}

/***************************************************************************
* void signalHandler(int signal)
* Author: Dylan Kreth
* Date: 3/1/2020
* Description: handles signals when they come in
* Parameters:
* signal I/P the signal to be received
* convertToMicroseconds O/P int number of microseconds
**************************************************************************/
void signalHandler(int signal)
{
	//update thisPack with data so it'll be correct when we send it to the parent
	thisPack.count = count;
	thisPack.pid = getpid();
	//TODO send message (id, count) on pipe, where parent will receive it
	write(fd[WRITE], &thisPack, sizeof(thisPack));
	count = 0; // reset count for new round
}

//TODO documentation
void waitOnPotato(mqd_t mq){
	char* junk; //temp ptr to fill with the data from the msg queue
	//TODO wait for potato (just read from message queue -- it's a blocking call)
	if(DEBUG) cout << "CHILD" << getpid() << ": trying to recieve potato" << endl;
	mq_receive(mq,junk,sizeof(junk),0);

}

void writeHeader(ofstream filestream, pack_t thisPack){
	filestream << "LOSER DETECTED: " <<thisPack.pid << " " << thisPack.count << endl; //TODO store loser info (info came in on the pipe)
	filestream << "\nROUND FINISHED\nRESULTS FROM THIS ROUND:\n\tPID\tCOUNT" << endl;
}
