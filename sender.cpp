#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 */
void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	/* TODO: 
        1. Create a file called keyfile.txt containing string "Hello world" (you may do
 		    so manually or from the code).
	    2. Use ftok("keyfile.txt", 'a') in order to generate the key.
		3. Use the key in the TODO's below. Use the same key for the queue
		    and the shared memory segment. This also serves to illustrate the difference
		    between the key and the id used in message queues and shared memory. The id
		    for any System V objest (i.e. message queues, shared memory, and sempahores) 
		    is unique system-wide among all SYstem V objects. Two objects, on the other hand,
		    may have the same key.
	 */
	//ftok("keyfile.txt", 'a')
	key_t key = ftok("keyfile.txt",'a');
	
	/* Make sure the key generation succeeded */
	if(key < 0)
	{
		perror("ftok");
		exit(-1);
	}

	
	/* TODO: Get the id of the shared memory segment. The size of the segment 				must be SHARED_MEMORY_CHUNK_SIZE */
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, IPC_EXCL | S_IRUSR | S_IWUSR); 
	/* Failed to allocate shared memory */
	if(shmid < 0)
	{
		perror("shmget");
		exit(-1);
	}


	/* TODO: Attach to the shared memory */
	sharedMemPtr = (char*)shmat(shmid,0,0);

	/* Make sure the memory was successfully attached*/
	if(((void*)sharedMemPtr) < 0)
	{
		perror("shmat");
		exit(-1);
	}

	/* TODO: Attach to the message queue */
	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */
	msqid = msgget(key, 0666);

	/* Make sure the queue was successfully created */
	if(msqid < 0)
	{
		perror("msgget");
		exit(-1);
	}
	
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	if(shmdt(sharedMemPtr) < 0)
	{
		perror("shmdt");
		exit(-1);
	}
	
	/* Deallocate the memory segment */
	if(shmctl(shmid, IPC_RMID, NULL) < 0)
	{
		perror("shmctl");
		exit(-1);
	}
	
	/*Deallocate the message queue */
	if(msgctl(msqid,IPC_RMID, NULL) < 0)
	{
		perror("msgctl");
		exit(-1);	
	}

}

/**
 * The main send function
 * @param fileName - the name of the file
 * @return - the number of bytes sent
 */
unsigned long sendFile(const char* fileName)
{
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");
	

	/* A buffer to store message we will send to the receiver. */
	message sndMsg; 
	
	/* A buffer to store message received from the receiver. */
	ackMessage rcvMsg;
	
	/* The number of bytes sent */
	unsigned long numBytesSent = 0;
	
	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}
	
	/* Read the whole file */
	while(!feof(fp))
	{
		//fprintf(stderr, "mehh\n");
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory. 
 		 * fread will return how many bytes it has actually read (since the last chunk may be less
 		 * than SHARED_MEMORY_CHUNK_SIZE).
 		 */
		if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
		{
			perror("fread");
			exit(-1);
		}
	
		/* TODO: count the number of bytes sent. */		
		numBytesSent += sndMsg.size;

		/* TODO: Send a message to the receiver telling him that the data is ready 
 		 * (message of type SENDER_DATA_TYPE) 
 		 */
		sndMsg.mtype = SENDER_DATA_TYPE;
		
		/*Check if message was sent*/
		if(msgsnd(msqid, &sndMsg, sizeof(struct message) - sizeof(long), 0) < 0)
		{
			perror("msgsnd");
			exit(-1);
		}
		/* TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us 
 		 * that he finished saving the memory chunk. 
 		 */
		msgrcv(msqid,&rcvMsg, sizeof(ackMessage) - sizeof(long),RECV_DONE_TYPE,0);
		
	}
	
	/** TODO: once we are out of the above loop, we have finished sending the file.
 	  * Lets tell the receiver that we have nothing more to send. We will do this by
 	  * sending a message of type SENDER_DATA_TYPE with size field set to 0. 	
	  */

	sndMsg.size = 0;
	/*Make sure the message was sent*/
	if(msgsnd(msqid, &sndMsg, sizeof(struct message) - sizeof(long), 0) < 0)
		{
		perror("msgsnd");
		exit(-1);
		}
	/* Close the file */
	fclose(fp);
	
	return numBytesSent;
}

/**
 * Used to send the name of the file to the receiver
 * @param fileName - the name of the file to send
 */
void sendFileName(const char* fileName)
{
	/* Get the length of the file name */
	int fileNameSize = strlen(fileName);

	/* TODO: Make sure the file name does not exceed the 
	 * the maximum buffer size in the fileNameMsg
	 * struct. If exceeds, then terminate with an error.
	 */
	if(fileNameSize > MAX_FILE_NAME_SIZE )
	{
		fprintf(stderr, "Your string is too big (should be <= MAX_FILE_NAME_SIZE characters)\n");
		exit(-1);
	}

	/* TODO: Create an instance of the struct representing the message
	 * containing the name of the file.
	 */
	fileNameMsg msg;

	/* TODO: Set the message type FILE_NAME_TRANSFER_TYPE */
	msg.mtype = FILE_NAME_TRANSFER_TYPE;

	/* TODO: Set the file name in the message */
	strncpy(msg.fileName, fileName, fileNameSize+1);

	//null terminate since strncpy doesn't always null terminate
	msg.fileName[fileNameSize] = '\0';	
	
	/* TODO: Send the message using msgsnd */
	if(msgsnd(msqid, &msg, sizeof(fileNameMsg) - sizeof(long), 0) < 0)
	{
		perror("msgsnd");
		exit(-1);
	}

}


int main(int argc, char** argv)
{
	
	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}
		
	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

	/* Send the name of the file */
        sendFileName(argv[1]);
		
	/* Send the file */
	fprintf(stderr, "The number of bytes sent is %lu\n", sendFile(argv[1]));
	
	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);
		
	return 0;
}
