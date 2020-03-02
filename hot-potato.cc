/*
 * Filename       hot-potato.cc
 * Date           3/1/2020
 * Author         Dylan Kreth
 * Email          dylan.kreth@utdallas.edu
 * Course         CS 4348.001 Spring 2020
 * Copyright      2020, All Rights Reserved
 * Procedures:
 * main - plays hot potato between 5 processes. when a process has caught the potato 1000 times, it explodes and 
 * the remaining processes play again
 * signalHandler - handles signals when they come in
 * openPipe - opens a pipe and processes any errors
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include "hot-potato.h"

using namespace std;


//Global variables bc they're accessed by signalHandler
volatile int count = 0; //for counting the number of potatoes each child has received
pack_t thisPack = {getpid(),0}; // holder for data to be sent on the pipe
int fd[2];//file descriptor for pipe

/***************************************************************************
* int main( int argc, char **argv )
* Author: Dylan Kreth
* Date: 2/24/2020
* Description:  plays hot potato between 5 processes. when a process has caught the potato 1000 times, it explodes and 
 * the remaining processes play again
*
* Parameters:
* argc I/P int The number of arguments on the command line
* argv I/P char *[] The arguments on the command line
* main O/P int Status code (not currently used)
**************************************************************************/
int main(int argc, char **argv)
{
	openPipe(fd); //open a pipe with the file descriptor
	mqd_t mq = mq_open(MQ_NAME, O_RDWR); // create message queue for potato passing (dak160130)

	// fork 5 times so that we have 5 children. be careful about if() statements so that we don't get too many forks
	pid_t pid = 1; //start it as nonzero so that the parent can do the initial fork
	int numChildren = 0; //used later to track how many children there are left in the game
	for (numChildren = 0; (numChildren < NUM_CHILDREN) && (pid != 0); numChildren++)
	{
		if(DEBUG) cout << "forking " << numChildren << endl;
		if((pid = fork()) < 0){ // fork returns negative int on error
			perror("fork"); //report fork error
			exit(1); //close bc program is hosed
		}
		if(pid !=0 && DEBUG3) cout << "created child" << pid << endl;
	}

	pid_t loserId; //to hold the id of the loser

	if (pid != 0) //parent process
	{
		ofstream filestream; //open a file to write results to
		filestream.open(RESULTS_FILE_NAME); //using const is best practice. defined in hot-potato.h
		close(fd[WRITE]); //we don't need the write end in the parent
		signal(SIGUSR1, SIG_IGN); //ignore this signal in the parent
		while(true){
			mq_send(mq,"stuff",sizeof("stuff"),0); // add potato to queue
				if(DEBUG2) cout << "PARENT: sent potato" << endl;
				if(DEBUG) cout << "in the parent" << getpid() <<endl;
				if(DEBUG2) cout << "PARENT: waiting for message on pipe" << endl;
			read(fd[READ], &thisPack, sizeof(pack_t)); // wait for a message on pipe (just read -- it's a blocking command). One will come in when someone hits 1000 catches
				if(DEBUG2) cout << "PARENT: got a message on pipe. writing to file" << endl;
			filestream << "\n===============================\nLOSER DETECTED: child" <<thisPack.pid << " with count " << thisPack.count << endl; // store loser info (info came in on the pipe)
			filestream << "ROUND FINISHED\nRESULTS FROM THIS ROUND:\n\tPID\tCOUNT" << endl; //column headers
			loserId = thisPack.pid; //grab the loserId
			if(DEBUG3) cout << "PARENT: sending signal" << endl;
			kill(0,SIGUSR1);// send SIGUSR1 signal to all children. loser will terminate, rest will send their counts to the parent via the pipe (then reset count to zero)
			if(DEBUG3) cout << "PARENT: signal sent" << endl;
			// for loop thru children, reading from pipe for each and recording their id and catch count
			for(int i = 0; i < numChildren; i++){
				read(fd[READ], &thisPack, sizeof(pack_t)); //read from the pipe
				filestream << "\t" << thisPack.pid << "\t" << thisPack.count << endl; //print id and results (count) of each child
			}
			numChildren--; //we should now have 1 fewer children
			if(numChildren <= 1) 
				break;//once we have 1 child, we're done!
		}

		//these lines break the program, so we're leaving them out
		// kill(0,SIGUSR1);//send signal to get winner
		// read(fd[READ], &thisPack, sizeof(thisPack)); //read from the pipe
		// filestream << "WINNER: " << thisPack.pid << endl;

		int* x; //for saving status into
		waitpid(loserId, x, 0); // wait for last child(ren) to terminate to prevent broken pipe
		// if(mq_close(mq) < 0){// unlink the message queue
		// 	cerr << "error closing the message queue. " << strerror(errno) << endl; //report the mq_close error
		// }
		filestream.close(); //we're done writing
	}

	else // one of the child processes
	{
		close(fd[READ]); //we don't need to write from the child
		signal(SIGUSR1, signalHandler); //establish SIGUSR1 handler
		if(DEBUG) cout << "CHILD" << getpid() << ": set up signalHandler" << endl;
		while(true){
			waitOnPotato(mq); // wait for a potato by trying to read from the queue
			count++; // increment counter bc we caught a potato
			if(DEBUG2) cout << "CHILD" << getpid() << ": got a potato. count is now " << count << endl;
			if(count >= MAX_COUNT){ //potato explodes, this process just lost!
				if(DEBUG2) cout << "CHILD" << getpid() << ": my count hit 1000" << endl;
				thisPack.count = count; //update thisPack with data so it'll be correct when we send it to the parent
				thisPack.pid = getpid(); //update thisPack with data so it'll be correct when we send it to the parent
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
	

	return 0; // success!!
}

/***************************************************************************
* void signalHandler(int signal)
* Author: Dylan Kreth
* Date: 3/1/2020
* Description: handles signals when they come in
* Parameters:
* signal I/P the signal to be received
**************************************************************************/
void signalHandler(int signal)
{
	thisPack.count = count; // update thisPack with data so it'll be correct when we send it to the parent
	thisPack.pid = getpid(); // update thisPack with data so it'll be correct when we send it to the parent
	write(fd[WRITE], &thisPack, sizeof(thisPack)); // send message (id, count) on pipe, where parent will receive it
	count = 0; // reset count for new round
}

/***************************************************************************
* void waitOnPotato(mqd_t mq)
* Author: Dylan Kreth
* Date: 3/1/2020
* Description: waits on the message queue, trying to pull a potato
* Parameters:
* mq I/P the message queue to read from
**************************************************************************/
void waitOnPotato(mqd_t mq){
	char* junk; //temp ptr to fill with the data from the msg queue
	if(DEBUG) cout << "CHILD" << getpid() << ": trying to recieve potato" << endl;
	mq_receive(mq,junk,sizeof(junk),0); // wait for potato (just read from message queue -- it's a blocking call)

}

/***************************************************************************
* void openPipe(int* fd)
* Author: Dylan Kreth
* Date: 3/1/2020
* Description: opens a pipe and processes any errors
* Parameters:
* fd I/P pointer to the array with the file descripter for the pipe
**************************************************************************/
void openPipe(int* fd){
	//open a pipe for data transfer from child to parent
	if (pipe(fd) == -1) //pipe() returns -1 on error
	{
		cerr << "pipe() returned -1. " << endl; //errors should be reported
		perror("pipe");													//send a pipe error
		exit(EXIT_FAILURE);											// if pipe fails, project is hosed, so just exit
	}
}
