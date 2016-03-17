
#include <stdio.h>
#include <stdlib.h>
/*
#include <string.h>
#include <pthread.h>
*/

#include "sharedMemory.h"

//
//	c a n M e s s a g e . c
//
extern unsigned int verbose = 0;

//
// NOTE: All references (and pointers) to data in the can message buffer is
// performed by using the index into the array of messages.  All of these
// references need to be relocatable references so this will work in multiple
// processes that have their shared memory segments mapped to different base
// addresses.
//


//
//	I n s e r t M e s s a g e 
//
// Insert a new message into the message buffer.
//
int insertMessage ( struct canMessage_t* message )
{
	canMessageIndex_t newIndex = 0;

	//
	// Acquire the lock on the shared memory segment.
	//
	pthread_mutex_lock(&semap->lock);
	semaphore_wait ( &sharedMemory->lock );

	//
	// If there are no more CAN message buffers available, complain and
	// quit.
	//
	if ( sharedMemory->freeListCount == 0 )
	{
		printf ( "No more CAN message blocks available.\n" );
		return 0;
	}
	//
	// Grab the head of the free list as our new message block.
	//
	newIndex = sharedMemory->freeListHead;

	canMessage_t* buffer = &sharedMemory->bufferBase[newIndex];
	sharedMemory->freeListHead = buffer->nextMessageIndex;

	//
	// Copy the message ID and it's data into the new message buffer.
	//
	buffer->messageId = message->messageId;
	buffer->data      = message->data;

	//
	// Decrement the number of CAN message blocks that are left now.
	//
	--sharedMemory->freeListCount;

	//
	// Give up the shared memory block lock.
	//
	semaphore_post ( &sharedMemory->semaphore );

	//
	// Return the index of the new CAN message block to the caller.
	//
	return newIndex;
}


//
//	f e t c h M e s s a g e 
//
// Fetch an existing message from the message buffer.
//
int fetchMessage ( struct canMessage_t* message, canMessageIndex_t id )
{
	canMessage_t* fetchedMessage;

	//
	// Acquire the lock on the shared memory segment.
	//
	// semaphore_wait ( &sharedMemory->semaphore );
	pthread_mutex_lock(&sharedMemory->semaphore.lock);

	// fetchedMessage = &sharedMemory->bufferBase[id];
	fetchedMessage = &sharedMemory->bufferBase[1];

	if ( fetchedMessage->flag != 0 )
	{
		printf ( "Fetching message in use\n" );
		return 0;
	}
	fetchedMessage->flag = 55555555;

#if 0
	if ( fetchedMessage->messageId != id || fetchedMessage->data != id )
	{
		printf ( "Error fetching message - Expected id %'u, got %'u\n", id,
				 fetchedMessage->messageId );
		return 0;
	}
#endif
	//
	// Copy the message ID and it's data into the new message buffer.
	//
	// buffer->messageId = message->messageId;
	// buffer->data      = message->data;
	for ( int j = 0; j < 10000; j++ )
	{
		canMessage_t* dummyBuffer;
		dummyBuffer = &sharedMemory->bufferBase[j];
	}
	fetchedMessage->flag = 0;
	//
	// Give up the shared memory block lock.
	//
	// semaphore_post ( &sharedMemory->semaphore );
	pthread_mutex_unlock(&sharedMemory->semaphore.lock);

	//
	// Return the index of the new CAN message block to the caller.
	//
	return id;
}


#if 0
//
// Remove a message from the message buffer.
//
int deleteMessage ( canMessageId_t messageId )
{

	semaphore_wait ( hashTableEntry.accessSemaphore );

	hashTableEntry_t hashTableEntry& = hashTable[messageId];
	hashTableEntry_t currentListHead& = hashTableEntry.messageIndex;

	hashTableEntry.messageIndex = bufferPtr[hashTableEntry.messageIndex]messageIndex;

	semaphore_post ( hashTableEntry.accessSemaphore );
}

#endif
