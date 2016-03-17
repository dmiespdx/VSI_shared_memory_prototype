#pragma once
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "canMessage.h"

//
// Note: All references (and pointers) to data in the data message pool are
// performed by using the index into the array of messages.  All of these
// references need to be relocatable references so this will work in multiple
// processes that have their shared memory segments mapped to different base
// addresses.
//

//
// Define the shared memory segment that will be shared among multiple
// processes.  This structure will be mapped into a shared memory segment for
// all processes to use.
//
typedef struct sharedMemory_t
{
	//
	// Define the global constants and variables.
	//
	unsigned int totalMessageCount;
	unsigned int totalSharedMemorySize;

	//
	// The free buffers (those not currently in use by a process) will be kept
	// on a simple singly linked list.  Buffers are removed from the head of
	// the list and added to the tail.
	//
	// [This feature is not implemented for this demo code.]
	//
	int               freeListCount;
	canMessageIndex_t freeListHead;
	canMessageIndex_t freeListTail;

	//
	// Define the global shared memory lock.
	//
    pthread_mutexattr_t mutexAttributes;
	pthread_mutex_t     lock;

	//
	// Define the start of the array of CAN message records.  This array will
	// be created with a specified size before anyone tries to read or write
	// any of the records in it.
	//
	canMessage_t messagePoolBase[0];

}   sharedMemory_t;

//
// Define the address of the global shared memory segment.  This value will be
// filled in when the shared memory segment file is mapped into the virtual
// memory space.
//
static sharedMemory_t* sharedMemory;

//
// Define the name of the shared memory segment.
//
static const char* sharedMemoryName = "/var/run/shm/CanSharedMemorySegment";

//
// Define the size of the shared memory segment.  Note that this is a
// calculation that will be performed at runtime when the shared memory
// segment is loaded.  The calculation is the size of the shared memory
// structure plus the size of the CAN message pool (which is calculated
// based on a runtime parameter that is supplied when the system starts).
//
static unsigned int sharedMemorySize;

//
// Define the member functions.
//
sharedMemory_t* sharedMemoryOpen ( void );
void            sharedMemoryClose ( sharedMemory_t* sharedMemory,
									unsigned int sharedMemorySegmentSize );
unsigned int    sharedMemoryGetSegmentSize ( sharedMemory_t* sharedMemory );
unsigned int    sharedMemoryGetPoolSize ( sharedMemory_t* sharedMemory );

void            sharedMemoryLock ( void );
void            sharedMemoryUnlock ( void );

//
// Message manipulation functions.
//
int insertMessage ( struct canMessage_t* message );
int fetchMessage  ( struct canMessage_t* message );


#endif		// End of SHARED_MEMORY_H
